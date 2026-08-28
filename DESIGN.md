
## 1. Overview

This is an RTL design project that creates from scratch a RISC-V RV32I CPU and a two layer LIF SNN (Leaky Integrate and Fire Spiking Neuron Network) accelerator built as a peripheral of the CPU core.

This project demonstrates:
1. A complete vertical stack of a SNN accelerator
    a. RISC-V RV32I core that passed the RISC-V official rv32ui test
    b. Trained 2 layer LIF SNN model with MNIST dataset (97.6% test accuracy) for image classification that produced the golden reference model for the inference accelerator.
    c. Custom accelerator that processed testcases with 28x fewer cycles (75510 vs 2127999 cycles on core)
    d. Thorough unit and datapath verification using C++, Assembly, and the golden reference model.
2. Statically structured system with cycle counts dependent only on the input data.
    a. Every datapath is fixed, so no cycle has two possible next actions.

## 2. System Architecture

### Memory map

Instruction Space  (fetched by PC)

|Address| Size | Region | Access |
| ------|-------- | -------  | ------- |
|0x0000_0000 – 0x0000_0FFF|4 KB |imem|R|

Data Space  (load/store)

|Address| Size | Region | Access |
| ------|-------- | -------  | ------- |
|0x0000_0000 – 0x0000_0FFF| 4KB | dmem | R/W|
|0x1000_0000| 4B | PRINT | W |
|0x1000_0004| 4B| EXIT| W|
|0x1000_0008| 4B| PRINT_INT| W|
|0x2000_0000 – 0x2000_0FFF| 4KB | accel control| R/W|
|0x2000_1000 – 0x2000_1FFF| 4KB| event bank A|W|
|0x2000_2000 – 0x2000_2FFF| 4KB| event bank B|W|
|0x2002_0000 – 0x2003_FFFF| 128KB |w1| W|
|0x2004_0000 – 0x2004_0FFF| 4KB |w2| W|

### Host & Accelerator Interface

Registers
All within the accelerator control region, so addresses are `0x2000_0000 + offset`.

|Address| Name | Kind | Access | Bits | Meaning |
| ------|-------- | -------  | ------- |------- |------- |
|0x2000_0010 |T|config| W | 5 |timesteps per image, 1..31|
|0x2000_0014 |VTH1 |config | W | 16 | layer 1 threshold, signed|
|0x2000_0018 |VTH2 |config | W | 16 |layer 2 threshold, signed|
|0x2000_001C | K |config| W | 3 | parameter for leak shift, 1..5|
|0x2000_0020 | START |doorbell|W| — |  begin image pulse, doorbell data ignored|
|0x2000_0024 | EVA_LEN |config, per timestep|W | 10 | event count for bank A, 0..784; writing to it also marks bank full
|0x2000_0028 | EVB_LEN|config, per timestep| W | 10 | event count for bank B, 0..784; writing to it also marks bank full
|0x2000_002C | STATUS| status| R | 3 |3bits: {image_done, bank_b_free, bank_a_free}|
|0x2000_0030 |H_SHIFT| config | W | 3 |  ev_rd_data shift for generating w1 addr and layer-1 sweep counter limit|
|0x2000_0040 + 4w | COUNT[w]|result | R | 32 | four counts per word; each byte = `4w+lane`;w = 0..2

Shared Regions

|Region| Memory (expected) | Written By | Read By | Concurrent |
| ------|-------- | -------  | ------- |------- |
|ev bank A |LUTRAM| host, per timestep|drain| never (2 separate banks)|
|ev bank B |LUTRAM| host, per timestep| drain| never (2 separate banks)|
|w1|BRAM| preloaded before reset|drain|never (separate by stages) |
|w2|BRAM| preloaded before reset|drain|never (separate by stages) |

Host contract:
- write event length after writing an event bank to mark bank full.
- read COUNT before writing START for the next image.

### Sequence

```
boot:       T, VTH1, VTH2, K
            w1/w2 preloaded before reset

per image:
            fill bank A with t=0's spike events, write EVA_LEN
            START
            for t in 0..T-1:
                wait STATUS.bank_free for the opposite of current timestep read bank
                fill it with timestep t+1's spike events
                write that bank's LEN
            wait STATUS.image_done
            read COUNT[0..2], unpack 10 counts, argmax on the host
```

## 3. Decisions & Rejected Alternatives

### 3.1 Tooling and environment

| # | Decision | Chose | Rejected | Because |
|---|---|---|---|---|
| D1 | FPGA board | Arty A7-100T | Basys3, PYNQ Z2, DE1-SoC, DE2-115 | No prewritten SoC, most balanced and accessible |
| D2 | Waveform viewer | GTKWave | Surfer, NovyWave | Mature ecosystem |
| D4 | HDL | Verilog-2001 | SystemVerilog | Harder to learn, more fundamental |
| D5 | Verification methodology | Directed C++ benches + riscv-tests + golden model | UVM / constrained-random | Covers what's needed without expensive license |

<details>
<summary>D1: What FPGA board to run on</summary>

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
Consider DE1-SoC for September target, to demonstrate vendor portability.

</details>

<details>
<summary>D2: What waveform viewer to use</summary>

**Chose:** GTKWave

**Rejected:** Surfer, NovyWave (other open source tools).

**Because:** GTKWave is the oldest, most documented waveform viewer, and it integrates friction-free with Verilator's trace formats. Most Verilator tutorials or discussions on online forums assume the use of GTKWave. Surfer has newer UI but less ecosystem maturity that is more important.


<small>Commercial tools like Synopsys Verdi are not considered due to the cost of license.</small>

**Revisit if:** Another waveform viewer provides significantly better performance or Verilator integration.

</details>

<details>
<summary>D4: What HDL to write in</summary>

**Chose:** Verilog-2001

**Rejected:** SystemVerilog (SV)

**Because:** The smallest common language every tool in the flow fully supports. SV is a superset of Verilog, so learning it afterward is incremental and easier than the reverse, as Verilog includes syntaxes that SV simplifies (combinational completeness, latch inference).

**Revisit if:** Future projects will likely use SV, this codebase stays Verilog-2001.

</details>

<details>
<summary>D5: What verification methodology to use</summary>

**Chose:** Directed C++ testbenches per RTL block, the official riscv-tests suite as an external referee for the core, golden reference model as the correct answer of the accelerator.

**Rejected:** UVM / constrained-random verification

**Because:** UVM pays off at team scale with verification IP, component isolation, and constrained-random coverage. However, it needs commercial simulators Verilator can't replace, and the functions it provides are not essential to a single cycle core.

**Revisit if:** A team-scale project with commercial seats, or a pipelined core, where hazard × forwarding × flush combinations create the state space of constrained-random.

</details>

### 3.2 Host core

| # | Decision | Chose | Rejected | Because |
|---|---|---|---|---|
| D3 | How to get a CPU host | Write RV32I from scratch | PicoRV32 / FemtoRV | Part of the goal, and good RTL practice for accelerator |
| D6 | Single-cycle or pipelined | Single-cycle | 5-stage pipeline | Current measuring metric |
| D7 | Base ISA or extensions | RV32I only | RV32IM, RV32IMC, custom ISA | Smallest core without multipliers |
| D8 | Where branch comparison lives | Dedicated comparator (branch.v) | Route through the ALU | Costs little hardware and doesn't need translation logic |

#### D3: How to obtain a CPU host
**Chose:** Write RV32I core from scratch.

**Rejected:** PicoRV32/FemtoRV as host.

**Because:** The project is the whole stack, and dropping in someone else's core leaves half of it unbuilt. Writing the CPU first also meant learning the RTL patterns the accelerator needed later, on a design with a published spec to check against.

**Revisit if:** Core verification wasn't complete by late July, then swap in PicoRV32 and the accelerator will be the entire project. Since core verification was completed, there will be no revisits.

#### D6: Single-cycle or pipelined host
**Chose:** Single-cycle.

**Rejected:** 5-stage pipeline.

**Because:** The metric used in this project is cycle count, and pipelining only raises clock speed. Pipelining also introduces the need to test instruction interactions with stalls and flushes, which doesn't align with the current verification approach of testing each instruction by itself.

**Revisit if:** The metric changes from cycle count to the actual time speed on the FPGA.

#### D7: Base ISA or extensions
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

#### D8: Where branch comparison lives
**Chose:** Dedicated comparator module (branch.v) beside the ALU; ALU idle during branches.

**Rejected:** Routing comparison through the ALU (textbook structure).

**Because:** The ALU was already verified at ten operations; routing branches through it reopens a verified block. The branch funct3 codes also don't map directly to the ALU's, so sharing also needs translation logic (the textbook wires branches to the ALU because it only implements BEQ, which needs just a subtract-and-check-zero). A dedicated comparator costs a small number of LUTs out of 63k of the Arty.

**Revisit if:** The core is pipelined.


### 3.3 Accelerator

#### A1: What type of input encoding to use
**Chose:** Rate coding

**Rejected:** Latency (temporal) coding

**Because:** Rate encoding is better for model robustness and training effectiveness. Latency encoding relies on brighter pixels spiking earlier, which means each spike carries more information and there are fewer spikes in total for the image. That makes the model noisier and harder to train, with the tradeoff of better energy efficiency. This is especially important to my model because it uses quantized training (QAT) for the accelerator's integer weight input: the less redundancy of spikes, the more quantization costs.

**Revisit if:** Input becomes dependent on spike timing. A static MNIST image is not.

#### A2: What is the reset policy
**Chose:** membrane potential subtract threshold (soft reset)

**Rejected:** membrane potential reset to zero (hard reset)

**Because:** The default snn.Leaky reset mechanism is subtract (snntorch's documentation notes that subtract performs better on a few benchmarks). Subtract preserves the previous timestep's magnitude, which is important for rate coding because rate coding depends on the spikes of every timestep, not just the first one (like latency coding). Choosing subtract over zero does not cost anything on the hardware side.

**Revisit if:** Membrane potential (v) is driven into saturation (the int16 width limit) often enough to distort the results.

#### A3: How to leak membrane potential in hardware
**Chose:** multiplicative leak `V *= β` implemented as one arithmetic shift-subtract,
`V -= V>>k`, which equals `V*(1 - 2^-k)`. So `β = 1 - 2^-k`.
(k=1→0.5, k=2→0.75, k=3→0.875, k=4→0.9375, k=5→0.96875)

**Rejected:** a general fixed-point multiply by an arbitrary β

**Because:** β is a hyperparameter with no correct answer, so there is no approximation error, and a shift can be implemented in the accelerator faster and with less hardware. An arbitrary β allows more options but costs a multiplier in the accelerator.

**Revisit if:** none of k=1..5 trains acceptably. All five did, so k=2 was chosen and will not be revisited.

#### A4: What number formats and accumulator width to use
**Chose:** weights `int8`, membrane v `int16`, accumulate arriving weights (acc) in an
`int32` register, then saturate (clamp) to int16 on writeback to v.

**Rejected:** accumulate arriving weights (acc) in an `int16` register and storing membrane v in `int32` register.

**Because:** for int8 weights, the measured values of v and acc stay well inside int16 (across all three models max|acc| = 2,402, max|v| = 8,874, see docs/img for value distribution). Acc is int32 because it has to cover the worst case: 784*127 = 99568 in the hidden layer, which exceeds int16's 32767. At that width it cannot overflow, so it needs no clamp. V is int16 because the leak and the threshold subtraction bound it, and the clamp on writeback catches anything past that.

The saturation clamps v>32767 to 32767 and v<-32768 to -32768 so that it doesn't wrap to the wrong signed values.

**Revisit if:** a re-measurement shows frequent clamping that rescaling weights can't fix, then widen v to `int32`. Experiment showed that clamping never triggered. Also worth revisiting with an int32 v once there is a Vivado setup, where the synthesis will reveal which one uses less hardware (int16 + clamp or int32 without clamp).

#### A5: Immediate or deferred reset timing
**Chose:** Immediate reset. On fire, subtract `V_th` on the same timestep the neuron
crosses threshold

**Rejected:** Deferred reset where the subtraction lands on the next timestep

**Because:** For deferred reset, it needs additional hardware to carry the pending reset bit per neuron to the next timestep, and the accuracy for deferred reset isn't better than immediate reset.

**Revisit if:** deferred reset trains materially better. Measured at h128 with the integer model: 97.02% deferred vs 97.63% immediate (float: 97.25% vs 97.59%), so there's no advantage. Will not be revisited.

## 4. Verification

### RISC-V core: riscv-tests results

| Metric | Value |
|---|---|
| rv32ui tests run | 40 |
| Passed | 40 |
| Failed | 0 |
| Excluded | 2 (ma_data, fence_i) |

Run against the unmodified upstream rv32ui test bodies, via a CSR/trap-free environment (verif/env) built for this core.

| Excluded test | Reason |
|---|---|
| ma_data | Checks the exact value of a misaligned load/store. Passing needs either real hardware support or a trap handler that decodes and emulates in software. The trap route also needs CSRs (mtvec/mepc/mcause), which the core doesn't have. The ISA allows trapping instead of hardware support, so excluding this is a scope decision, not a compliance gap. |
| fence_i | Zifencei extension, not base RV32I, not needed for compliance. Split I and D memories (Harvard), so storing can never reach imem. |

**Negative control:** Broke `add` (used everywhere, including boot) and all 40 tests failed; broke `or` and `xor` individually and only their own tests failed. Confirms the tohost signal detects real failures, not a rubber stamp.


