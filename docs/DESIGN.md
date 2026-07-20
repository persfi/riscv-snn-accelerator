

## 1. Decisions & Rejected Alternatives

### D1: What FPGA board to run on
**Chose:** Arty A7 100T 
**Rejected:** Basys3, PYNQ Z2, DE1-SoC, DE2-115
**Because:** 
||Arty A7 100T|Basys3 |DE2-115| DE1-SoC| PYNQ Z2|
| ------|-------- | -------  | ------- | ------- |---|
|LC/LE|101k|33k|114k|85k|85k|
|BRAM|4.8Mb|1.8Mb|3.8Mb|3.9Mb|4.9Mb|
|Tool|Vivado|Vivado|Quartus Prime|Quartus Prime|Vivado|
|Reason|Most balanced, accessible (in Taiwan), and within budget.| Same series as A7 100T but three times smaller. BRAM too small for the project.|Massive physical I/Os that aren't needed in this project. Harder to source than the Arty A7 100T.|Built in system on chip (SoC), used in my ECE241 course next term, displacing the RISC-V core which is a point of the project. |Built in SoC and Jupyter/Python environment. However, this project will not be running Python on the chip and will only execute C programs (Pytorch is traintime only).

<small>LC/LE units depends on the vendor and are not directly comparable; written for order-of-magnitude comparison only</small>

**Revisit if:** 
Reconsider PYNQ Z2 if accelerators, embedded vision, or AI inference became the project core. 
Consider DE1-SoC for September port target (accelerator fabric, own core stays as host), to demonstrate vendor portability. 


### D2: What waveform viewer to use
**Chose:** GTKWave 
**Rejected:** Surfer, NovyWave (other open source tools).
**Because:** GTKWave is the oldest, most documented waveform viewer, and it integrates friction-free with Verilator's trace formats. Most Verilator tutorials or discussions on online forums assume the use of GTKWave. Surfer's newer UI didn't outweigh ecosystem maturity.


<small>Commercial tools like Synopsys Verdi are not considered due to the cost of license.</small>

**Revisit if:** Another waveform viewer provides significantly better performance or Verilator integration.

### D3: How to obtain a CPU host 
**Chose:** Write RV32I core from scratch. 
**Rejected:** PicoRV32/FemtoRV as host.
**Because:** The project aims to build the whole vertical stack, and a prebuilt core would leave the host side of the claim empty. Implementing the CPU also serves as structured practice for designing the accelerator's unscaffolded RTL.
**Revisit if:** Core verification isn't converging by late July, then swap in PicoRV32 and let the accelerator be the entire project.

### D4: What HDL to write in
**Chose:** Verilog-2001
**Rejected:** SystemVerilog (SV)
**Because:** The smallest common language every tool in the flow fully supports. SV is a superset of Verilog, so learning it afterward is incremental; the reverse is harder, since Verilog forces the learner to face what SV simplifies away (combinational completeness, latch inference). 
**Revisit if:** Future projects will likely use SV, this codebase stays Verilog-2001.

### D5: What verification methodology to use
**Chose:** Directed C++ testbenches per RTL block, the official riscv-tests suite as an external referee for the core.
**Rejected:** UVM / constrained-random verification
**Because:** UVM pays off at team scale (verification IP, component isolation, constrained-random coverage), needs commercial simulators Verilator can't replace, and oversizes the problem: nothing carries over between instructions on a single-cycle core, so directed tests and the compliance suite already cover the space.
**Revisit if:** A team-scale project with commercial seats, or a pipelined core, where hazard × forwarding × flush combinations create the state space constrained-random was built for.

### D6: Single-cycle or pipelined host
**Chose:** Single-cycle.
**Rejected:** 5-stage pipeline.
**Because:** My metric is cycle count, and pipelining only raises clock speed, not cycle count. Hazards (stalls, flushes) could even raise it. Pipelining also means new hardware (pipeline registers, forwarding, hazard detection, flush logic) and new test cases for instructions now interacting across stages, which would double the verification surface for a metric this project doesn't use. Either way the same rv32i gcc output runs unchanged, so the choice costs nothing in software.
**Revisit if:** Never within this project. If timing trouble comes up, lower the clock instead of pipelining. 

### D7: Base ISA or extensions
**Chose:** RV32I only (-march=rv32i -mabi=ilp32).
**Rejected:** RV32IM, RV32IMC, custom ISA.
**Because:**
||RV32I|RV32IM|RV32IMC|Custom ISA|
|---|---|---|---|---|
|Multiply|Software (gcc libgcc)|Hardware|Hardware|Custom, undefined|
|Instruction width|Fixed 32-bit|Fixed 32-bit|Mixed 16/32-bit|Undefined|
|Toolchain|riscv64-unknown-elf-gcc, multilib (-march=rv32i)|Same toolchain (-march=rv32im)|Same toolchain (-march=rv32imc)|None, I'd write my own|
|riscv-tests compliance|Yes|Yes|Yes|No, no spec to conform to|
|Reason|Mostly loads/stores/branches, not math. Smallest ISA that still runs real C.|Multiply is rare here. A hardware multiplier would sit unused.|Two instruction sizes add decode complexity for no real benefit.|No real compiler or compliance suite to prove it runs C.|

**Revisit if:** Multiply turns out to be a bottleneck in the future, then M would be added first.

### D8: Where branch comparison lives
**Chose:** Dedicated comparator module (branch.v) beside the ALU; ALU idle during branches.
**Rejected:** Routing comparison through the ALU (textbook structure).
**Because:** The ALU was already verified at ten operations; routing branches through it reopens a verified block. The branch funct3 codes also don't map directly to the ALU's, so sharing also needs translation logic (the textbook wires branches to the ALU because it only implements BEQ, which needs just a subtract-and-check-zero). A dedicated comparator costs a small number of LUTs out of 63k.
**Revisit if:** The core is pipelined.

## 8. Verification

### RISC-V core: riscv-tests results

| Metric | Value |
|---|---|
| rv32ui tests run | 40 |
| Passed | 40 |
| Failed | 0 |
| Excluded | 2 (ma_data, fence_i) |

Run against the unmodified upstream rv32ui test bodies, via a minimal, CSR/trap-free environment (verif/env) built for this core.

| Excluded test | Reason |
|---|---|
| ma_data | Checks the exact value of a misaligned load/store. Passing needs either real hardware support or a trap handler that decodes and emulates in software. The trap route also needs CSRs (mtvec/mepc/mcause), which the core doesn't have. The ISA allows trapping instead of hardware support, so excluding this is a scope decision, not a compliance gap. |
| fence_i | Zifencei extension, not base RV32I, not needed for compliance. Split I and D memories (Harvard), so storing can never reach imem. |

**Negative control:** Broke `add` (used everywhere, including boot) and all 40 tests failed; broke `or` and `xor` individually and only their own tests failed. Confirms the tohost signal detects real failures, not a rubber stamp.


