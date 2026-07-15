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
