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
