# Engineering Log

## 2026-07-03
- Finished verilog essentials over the last week on HDLbits. (modules, gates, ffs & latches, ALUs, counters, shift registers)

## 2026-07-04
- Ordered Arty A7 100T (digikey tw). See D1 in DESIGN.md.
- Installed verilator, GTKwave (from apt not source / see D2 DESIGN.md), make, venv for snntorch, numpy, pytest, torch that'll be used later. Did not use Conda for managing packages as it only tracks Python packages, so it's unable to track verilator or toolchains etc that'll be needed in the project.
- cd to root of wsl from powershell can reset ubuntu's password directly (bypassing wsl).
- Editors track venv by the name .venv so named it .venv instead of any other names.
- Learned makefile basics. ( $(path shortcut) / .PHONY tells make that they are commands not files )
- tool_versions.md for all tool versions including and outside venv.
- Ubuntu apt update failed due to connection issue with ipv6 (new type of ip address), switched to ipv4 → works
- requirements.txt created with make freeze so python packages can be installed in the future in one command.
- pip install torch installs the whole stack including ones designated for gpus (which i don't have), uninstalled and installed --index-url .../whl/cpu saving space. 
- project originally created in mnt/c, figured operating it from wsl slows it down, tried mv them to wsl home. mv failed, files got deleted before git init → had to reinitialize the whole project (always git init first)

## 2026-07-05
- installed vivado 2026.1 version (needed for arty a7 100t see D1 DESIGN.md): stripped bare except for arty A7 and basic tools to save disk space. 2026 july version's license had an update → used basic tier node locked (tie to machine)
- dll error when launching vivado, installed visual c++ distributable for the missing 2015-2022 x64 thats causing the error → works.
- ethernet MAC for vivado host ID (don't have dongle flexid of enterprises, disk serial number more fragile to drive swaps)
- Vivado installed on windows as wsl doesnt have default usb port that'll be used for connecting my board.
- learn riscv assembly → till stacks.

## 2026-07-06
- installed 64 riscv toolchain from apt, quick and can still run 32 compile commands.
- smoke test with c file passed → toolchain works properly.
- studied RISC-V isa, RISC-V assembly, single cycle datapaths, control unit mapping table, and performance eval → can start writing rv32i's verilog tomorrow.
- add riscv tests as git submodule for testing later.

## 2026-07-07
- \+ verilog (&sv) extension in vscode.
- some datapath labels dont follow riscv naming → switched all of them to riscv to make it easier to cross check with encode/decode tables.
- write data with clk, read data continuosly so that instructions can be proccessed in a single cycle.
- ascending index range is the convention for arrays ex [0:31].
- wrote regfile.v (combinational read, x0 gated both sides, no reset)
- theres 32 registers bc in isa they define 5 bits for rs1 and rs2. 
- need to map truth tables for ALUs design from isa spec.
- added make lint → checked for errors for regfile → all fine
- enable inset final newline in vscode to prevent lint warnings of their absence
- harness written by claude, reviewed the functions → understood what each of them does so i can write tests properly(generate boilerplate but unit tests are mine to write)

## 2026-07-08
- in simulator testing, if assign an value, it does not update automatically like real silicons do(for continuous assignments). Have to call eval() to tell the simulatro to reevaluate (see regfile_tb.cpp).
- only the signals that appear in the port list get exposed as accessible fields on the generated Vregfile C++ class so internally declared signals cant be called with dut.
- call tb defined methods with tb and verilator defined objects with dut (tb.top)
- tried writing to rs1_data and check if it is correct after settle(), failed tests. Figured reason: I can only drive module input ports in test files. anything written to output ports will get discarded after eval().
- may be able to recieve the fpga board on 7/10
- regfile_tb 8/8 tests success. (vscode underlines import as errors because verilated.h is in /usr/share/verilator/include/verilated.h not in project repo. The file is included from makefile so there's actually no errors. → add the path to c_cpp_properties.json to remove the false red underline)
- use 4 bits for determining the type of operations in ALUs because rv32i uses funct3 (8 variations) and funct7[5] (1 bit, 2 variations) to set them apart. 8 < 10 variations <16(4bit) therefore uses 4 bits.
- weighed localparam or 'define for my constants file(defs.vh). Chose localparam because it's actually a constant while 'define is just a macro replacing texts(hard to debug if its duplicated, no type support and lint will not catch the error).
- built alu.v ran lint, failed → RTL_SOURCES only includes (.v) → added Irtl/core to search through core → lint suceeded

## 2026-07-09
- wrote alu_tb, all tests passed (29/29). As alu doesn't have clk, added a no-clk condition in tb_harness.
- because there's a vh file for alu in core, changed unit-test call to make sure it searches for vh's when they're encountered
- wrote vectors.s, got to actually memorize the commands and sytnaxs from building not reading (failed to assemble when I didn't put commas between operands and mixed in several '//'s)
- assmebly: label is just the name for the addr of the next instruction
- CC enforces testbench split settings in claude.md thats based on file type rather than actually test logic → amended claude.md
- lint warns unused bits for inst → not sure if I won't use those bits in the future → added local lint waiver at port → lint pass. Same for vh file, as alu params arent used → local waiver at include → lint pass. 
- imm_gen tb passed (14/14)

## 2026-07-10
- wrote pc rtl lint and tests (6/6) passed.
- wrote imem rtl lint and tests (6/6) passed.
- parameter N = \$clog2(DEPTH) in imem.v, $clog2 might be risky for quartus versions but it should be fine with vivado for now. change in the future if needed.
- $readmemh, default verilog system task, copies hex text file to memory array. 
- program.hex will need to load too many tests, and if harness fails to copy then it will be a hard to catch bug → considered plusarg but it only works for sim to read at runtime and not for synthesizers (no runtime concept) → used a pragma of verilator, created load hex in harness to handle write to mem for all sim tests, synth is unaffected as the pragma would be seen as comment and it will run readmemh instead.
- typhoon today, board not arriving. 
- no need for 0x in hex files. 
- no need to write assembly instructions for imem testing because it doesn't matter if the hex encoded from the instructions are correct, the function I'm testing for imem is if it reads hex file properly and truncates the index of addresses correctly.

## 2026-07-11
- harvard architecture: needed seperate imem and dmem. Built dmem without write, test (6/6) passed. finish all leaf files.
- error in assign in core file, then realized its not actually a problen with assign its forgetting to write the module name → always debug from the first occuring error.
- output signals can be read in modules→ never seen in hdlbits practice but apparently its common. 
- wrote core rtl for pc+imem wiring, testing passed (8/8).
- error when linked all leaf files in core, turns out its because of defs's define guard, when multiple modules that uses it are loaded into core, the preprocessing state of verilator remembers that macro was called before so it doesn't load again into the other leaf files. but each modules has their own scope and needs to include param independantly → remove the guard 
- define guard was C convention, to prevent duplicating parameters in the same translation unit(complete chunk of C or C++ code that the compiler processes in one compilation) if included twice. but modules have their own scope so it doesnt apply to verilog
- wrote lw path, lint and test (25/25 in total for core) passed.

## 2026-07-12
- wrote sw path, lint and test (31/31 in total for core) passed.
- don't put spaces around trace "=" or itll return a empty variable error
- upgraded harness and trace handling (via claude as it's more of a boilerplate) to make tb visualization cleaner and saves time writing tb. 
- r type path wires, lint and test (36/36 in total for core) passed.
- riscv64-unknown-elf-gcc only accepted 0-31 shamt, tried -4 but failed → changed to 4
- last rtype test left pc at 24, need a rst or it reads old instructions.
- wrote itype paths, lint and test (43/43 in total for core) passed.

## 2026-07-13
- fpga board arrived, connected via micro usb. power led fine, done led works, preprogramed led patterns work. Vivado device manager can also connect to it, it also shows up on my windows device manager as a usb com port → basic tests done.
- decided to add seperate branch comparator, would use a bit more LUTs but it reduces the chance of errors from merging them into other units like alus. 
- bne x1,x2,4 doesn't work because i dont have linker yet, which is supposed to calculate the relative offest and plug into placeholder after assembling into object file. switched the raw numbers to labels intead, which the assembler knows thier position.
- wrote branch leaf file and test passed (15/15). didn't test it with core because by testing it individually first the core testing have much less variables to consider.
- wired btype datapath and passed test (59/59 in total for core)

## 2026-07-14
- added jal type and test passed (69/69 in total for core)
- added a jump > 4096 decoding imm in imm_gen to see that it does work properly for j far jumps. as the imem depth is set to 1024(for now) in rtl, can't test it in core without changing the setting, so testing it seperately in imm_gen is faster → (16/16) passed
- .org is absolute address, not offset, so I'd have to subtract the address of jal its jumping from to make sure its > 2048 (>512 index so that it uses the leftmost bit)
- confused jalr type instruction with jal and branches. rs1+imm jumps to absolute addr not relative to current pc → change the imms → jumps to the correct location.
- wrote jalr datapath and passed test (76/76 in total for core)
- lui unknown op was tested 3 times in the past already, skipped for jalr.

## 2026-07-15
- commeted out old lui tests bc now I've implemented lui it wouldn't work anymore
- added lui datapath and test passed  (78/78 in total for core)
- added auipc datapath and test passed (84/84 in total for core)
- figured in rv32ui test (ma_data) includes misaligned load and store. currently my project does not include misaligned address handling. In order to pass the test i have to do hardware misalign handling (doensn't fully support traps: no skip handler) → Decided to exclude it for now to prioritize the accelerator and the scheduler as that is the main goal of the project and misaligned address shouldn't be an issue for the project either.
- wrote load_ext and unit test passed (22/22)
- wrote loads datapath and test passed (92/92 in total for core) (nearly forgot the negative imm test). 

## 2026-07-16
- added store type datapath and test passed (98/98 in total for core).

## 2026-07-17
- learn more about linker concepts and how it organizes sections in tests (my tb didnt use it in previous tests because its just one .text file) → not going to use riscv tests linker because it starts with spike's(a riscv simulator) address base. using my own riscv test.h because theirs involve M-privilege mode's csr instructions. My core is a simple bare metal with no previledge setup, as my accelerator does not need any of them.
- using my own custum loading but the test vectors are tohost pass/fail check are still riscv's.
- generated linker and harness and passed all riscv tests(rv32ui) excluding ma_data and fence_i. 40/40 paseed, 2 excluded.
- vscode autoformats the .h as c and deletes the blank space between .section .text.init; use clang format off/on guard to prevent it happening again. 
- needs to prove the tst does report FAIL back tmrw by deliberately creating some errors in the rtl.

## 2026-07-18
- did negative test, changed add → all failed (everything uses add including env boot). cjanged or and xor → only their own tests failed → proved that the test reflects correct pass/fail.
- updated d4,d5 in DESIGN.md

## 2026-07-19
- added design decisions made during the building of rtl (D6, D7, D8).
- added riscv test results in DESIGN.md.
- learned about ctr0

## 2026-07-20
- updated riscv test results in DESIGN.md.
- seperated dmem from core so that soc can control and connect to accerator in the future.
- print and exit are not part of dmem and dmem and imem are different memory that BOTH have everything the linker defines.
- print only prints 1 byte.

## 2026-07-21
- updates soc harness via CC (infrastructure)
- wrote harness_test.S to test print and exit, both works.
- caps S so I can use define/include/any thing that needs to run with preprocessor
- li loads 32 bit immediate (lui+addi) 
- added crt0 and linker. inf loop in asm can be written as 1: j 1b (jump backwards to label 1)

## 2026-07-22
- x1 is ra that ret jumps back to, shouldn't be used to save other values in the assembly.
- la = load address = auipc + addi. sometimes the linker collapes 2 instruction from a pseudo instruction to one(linker relaxation). ex, only loading 12 bits is enough
- assemblers current section default to .text, since text init is declare in crt0, crt0 is placed in section .text.init
- tested hello.c and computation.c, hello prints hello world (71cycles), computation.c computes dot product of arrray (185cycles). switched * to + in computation.c cycles decrease to 73 (185-73=112; 112/5=22.4), around +23 for multiplication (as my core is rv32i not rv32im theres no dedicated multiplier so it uses a combination of current instructions)(calls to __mulsi3, a software routine in libgcc)
- avergae latency for multiplication is 22.4 but it is data dependant so each multiplication costs different cycles
- added assert at _end to make sure the theres at least 256 bytes between the end and stack top. 
- switched the stack top linker file order to the bottom, to prevent future confusion of the fact that it is higher than _end.

## 2026-07-23
- go through snn torch tutorials.
- snn neurons pass down either spike or non-spike, spike = 1*weight = w, non-spike = 0, no multiplication needed
- snn data is the most well documented and tested mnist, its 28x28=784 pixels, each has a probability of spike event depending on its pixel values. per event cost is fixed → is the next layer has 128 neurons, it will do 128 accumulative additions to the membrance of each neuron (1*weight = wight. where theres 128 weights). 

## 2026-07-24
- post training quantization for trained weights in software before loading them into mem bc it doesn't take floats.
- wrote lif (leaky integrate and fire) specs, pre designed number of bits of each symbols
- soft reset for exceeding threshold, decision docs in lif spec, which will be integrated into design md soon.
- leak constant beta will be done with V -= V>>k, which can be done without multipliers and as beta is a hyperparameter it is free to set. 
- if accumalation still exceeds 16bits after subtracting soft reset, clamp it within 16bits. safety net for overflow that might result in sign flips, but if clamping occurs too often or it visibly affects model accuracy, i should change weight scaling and make it smaller.
- leak (decay) with old membrane first before accumulation then threshold detection.
- spike encoding decision.
- before running accelerator, total cycle count should be predictable (predictable after ptq, encoding, and scheduler toolchain).

## 2026-07-25
- did lif_probe to check snn runs the steps as it says in its docs, leak>accumulate>fire, and checked that reset delay false does perform the reset immediately in the same timestep → all test passed
- chose immediate reset timing → no real benefit for delay which needs to forward another bit for previous state
- decided qat over ptq. qat trains what the hardware deploys and has better accuracy with slightly more complicated software (STE)
- installed torchvision for handling mnist and matplotlib for loss epoch graphing later.
- accidentally deleted a line of the snntorch library while reading it → fixed it. remember not to edit packages.
- no bias for the linear model because theres no continuous current that needs to be added in the lif equation

## 2026-07-26
- the forward of ste and forward of ratesnn is different. the ste one does the integer rounding while the ratesnn forward is the actual one that defines forward pass between all layers
- trained snn, 0.9762 test accuracy for k=2
- init_leaky is deprecated → replaced by reset_mem
- reset_mem resets the membrane per sample; each sample within a batch is trained in parallel with the others; samples are trained from T:0-20 before moving on to the next batch.
- done snn training apart from k sweep to choose the one with the best accuracy performance

## 2026-07-27
- loop through k from 1-5, chose k=2 for the leak beta because it has the second higher val accuracy compared to k=4 but way more stable that k=4 over epochs. although the differences in accuracy between the k varies very little so its not an important decision
- created golden model (rate_code, lif, and golden)
- used rng with seed for spike encoding, but realized that it differs if the batch differs, because it goes through every image per timestep before moving on to the next timestep, so if batch size increases, it will go through extra images (pixels) and the order of the generation will differ → switched to Thomas Wang's 32-bit integer hash generation method. because u is uniformly distributed from 0 to 255, the condition u < pixel creates a Bernoulli spike with probability pixel / 256 → (seed, image, timestep, pixel) ensures every pixel produced the same u
- batch doesnt matter in golden bc its pure inference
- decided that encoding and first quueuing of spikes should be done by c in core, or else its a whole new chunk of rtl. if there's time after completion, i can build it on the accelerator.
- the accel is a fsm and the core should only boot it and read its result
- exported golden model vectors for accel check in the future.
- check golden model against training snn and verified that the accuracy does not much with minor differences in weights, threholds, leak etc (that comes from converting float to int system)

## 2026-07-28
- studying possible accelerator design, figured that the counts.hex was in 32 bits, while the max count for each image is only 20(0or1 per timestep) → change to 8 bits

## 2026-07-29
- weight takes 1\*(784\*128+128*10) =101632bytes, 101632\*8/64=12704 lutram. only slicem luts can be used as lutram, which allows combianational read values from address. The utilization of lutram can only be seen during synthesis of the fpga, but since its a rather large amount of lutrams, directing it to bram would be safer, in case it doesnt work during synthesis. bram can only do synchronous read, so every read needs 1 clk cycle.
- decided to put the 128/10 v_i[t] (membrane) and acc in register. arty a7 100t has 126800 flipflops, accumulation needs 128\*32 while membrance needs 128\*16, core uses 32\*32 → 7168 in total, thats only 5.7%
- decided to load 1 word(4weights) of weights from bram every cycle, so if acc and v is in bram too itll need 3 more cycles to complete the read and perform combinational add with the weights → register allows combinational read → solves problem.
- ev_idx should be stored as lutram so it could be read combinationally. its not a register bc reg are independant ev_idx
- drew acellrator drain logic datapath

## 2026-07-30
- finish datapath design apart from sequenecer because it connects to too many things on the datapath and drawing it would just make the graph more difficult to interpret visually.
- leak and fire combines in lif unit. doesnt actually need to leak then drain then fire in hardware path, as long as the leak and fire's required data(v, acc) doesnt get over written it can be put together.
- acc_mem = reg / weight_mem = bram / v_mem = reg / ev_mem = lutram / spk1 = lutram
- sweep and drain all process 4 neurons at once (takes 1 word from address)
- spike push: 4 lanes can fire in one cycle but spk1 takes one index/cycle. loop i
over pending[3:0], stall_en holds word_cnt counters to prevent it from overwriting v. worst case is 5 cycles/group which is highly unlikely (4 consecutive fires)
- acc_mem is shared between layer1 and layer2, vmem cant becasue vmem persists through timesteps so theres v1,v2
- ev_mem gets 2 banks so host fills B while accel drains A -> so that accel doesnt have to wait for host to fill from t=1-19
- weight address is {idx, word_cnt}, a concatenation, not i*32+j — works because 32 is <<5 and word_cnt = [4:0]. by concatenating them its equivalent to shift idx <<5 and + cnt
- sequencer state is 2 orthogonal bits: layer_state (0=L1) and act_state (0=drain),plus outer states for idle/wait/done.
- accel keeps its own t because its gap between core t varies from 1 to 2
- spk1_wr_ptr also serves as spk1_len bc after existing the push to queue nothign updates it and it holds spk1_len.
- add mmio ports for accel
-  the host only encodes and fills the ev mem whenever theres space: its t has no relation to what stage the accel is at

## 2026-07-31
- changed mmio. ev and weight should be passed to accel by mmio and accel stores it in its mem array (which are lutram and bram respectively). 
- uses BASE + 4u*(i) so each word has its own addr, and that addr decodes into the idx of the mem array. the addr doesnt hold anything, only the array does.
- chose that over an auto-increment port (one fixed addr + ptr) bc direct addressing is readable to host, accel would also be free of the ptr reg.
- updated design decisions notes thatll be written to design md soon

## 2026-08-01
- accel should be able to process models with a different hdden layer configuration (diff number of neurons). added accel_l1_shift to mmio, shift is for the weight addr decoding so accel doesnt have to do it by itself. shift 
- _stack_top is actually just outside dmem and imem.
- design md memory maps and host accel interface table and use sequences.

## 2026-08-02
- wrote weight memory rtl for accelerator.
- read data should also be <= non blocking as its bram so theres a read delay. And it should not be in a condition check because the mem should read no matter if its write or drain state, only the acc mem chooses not to udpate.
- added accel scripts to lint harness.
- w_mem_tb test 8/8 passed; weight_mem_tb test 5/5 passed.
- forgot to check the read via layer_state, added it using read_at function to prove that the state select works.

## 2026-08-03
- wrote lif_unit rtl lint passed and test 19/19 passed.
- forgot to check the v_th and k assignments that prove that they aren't hardcoded → added them → test passed
- need another top module like the w_mem and weightmeme herirachy to control the layers state for lif_unit (k and v_th differs), also to control the stall and write spike to queue logic.
- negative arithmetic shifts round away from zero when there is a remainder.
- need to put $signed around part select v/acc and sign extended v/acc
- my 'for' is procedural (uses = for combinational reads) in lif_unit so it needs to be inside 'always'.
- moved DESIGN.md to root for visibility.

## 2026-08-04
- wrote counter lint and test 8/8 passed.
- counter should be like w_mem, with a top module instantaniate diff counts.
- mem in acc_mem is an unpacked array, can't part select across words → change it into a loop inside always → lint clean
- wrote acc_mem lint and test 11/11 passed.
- folded we and clear into a 2 bit ctrl so both cant be high at once
- wrote acc_rdata[i+:32] instead of [i*32+:32] , resulting in it reading overlapping bits → caught the bug and fixed it
- at first I wrote clear as 'else' after write, causing mem to clear whenever it's not writing. Might be harmless but more likely a bug in the future when theres cycles in drain thats not writing. → changed it to the 2 bit ctrl for safety.

## 2026-08-05
- wrote v_mem lint and test 15/15 passed
- combined v1 and v2 mem as it's small enough and when sequencer orders clean it should clean both at once, combining them simplifies the rtl.
- word_cnt needs to cap to 2 for the output layer (10 neurons, pad to 12, thats 3 words so 0,1,2 for word_cnt)
- originally used 8 bit for base but the lanes*word_cnt is 32 bits which caused an error → widen base to 32 bits → fixed the error

## 2026-08-06
- wrote spk1.v lint and test 6/6 passed.
- the sequencer handles wr_ptr (wr_addr)=0 and end which is spklen, and we comes from lif sweep push logic. spk1 is a reg mem array and takes 1 index per cycle so thats a 7 bits input wr_data. rd_addr never read and write at the same time so it'll be reuse (addr) with wr_addr: sequencer decides what state its at 
- wrote ev_mem.v lint and test 8/8 passed.
- needs both wr and rd addr because they happen at the same time and points to different boxes. we also exist as not all core outputs that lands in ev mem address region are writes
- 0xaaaaaaaa is unsigned bc hex literal gets fit into int then unsigned int, 0xaaaaaaaa is beyond int so it becomes unsigned, which is why a cast to signed is needed.
- wrote drain.v lint and test 4/4 passed
- drain is pure alu, the addr generation and delay logic should be managed by sequencer.

## 2026-08-07
- changed export_vectors to emit weights that'll be loaded into accel (8bit and flat) → emitted the new w1 & w2 hex.
