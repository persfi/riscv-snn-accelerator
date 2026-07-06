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

