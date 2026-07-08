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
- call tb defined methods with td and verilator defined objects with dut (tb.top)
- tried writing to rs1_data and check if it is correct after settle(), failed tests. Figured reason: I can only drive module input ports in test files. anything written to output ports will get discarded after eval().
- may be able to recieve the fpga board on 7/10
- regfile_tb 8/8 tests success. (vscode underlines import as errors because verilated.h is in /usr/share/verilator/include/verilated.h not in project repo. The file is included from makefile so there's actually no errors. → add the path to c_cpp_properties.json to remove the false red underline)
- use 4 bits for determining the type of operations in ALUs because rv32i uses funct3 (8 variations) and funct7[5] (1 bit, 2 variations) to set them apart. 8 < 10 variations <16(4bit) therefore uses 4 bits.
- weighed localparam or 'define for my constants file(defs.vh). Chose localparam because it's actually a constant while 'define is just a macro replacing texts(hard to debug if its duplicated, no type support and lint will not catch the error).
- built alu.v ran lint, failed → RTL_SOURCES only includes (.v) → added Irtl/core to search through core → lint suceeded
