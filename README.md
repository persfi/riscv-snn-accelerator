# RISC-V CPU core + Statically-Scheduled SNN Accelerator

RV32I core driving an event-driven spiking-neural-network accelerator, scheduled at compile time.

> [!NOTE]
> Design rationale, memory map, and rejected alternatives are in **[DESIGN.md →](DESIGN.md)**

RTL is intentionally a minority of this repo. Verification outweighs the design, as it is in industry. (Verilator C++ harness, unit TBs per block, all exists in /verif. Excluded manually from the Github language bar.) 

Started July 2026.

## Quickstart
```bash
git clone --recursive https://github.com/persfi/riscv-snn-accelerator
cd riscv-snn-accelerator
```

If you already cloned the repository without `--recursive`:

```bash
cd riscv-snn-accelerator
git submodule update --init --recursive
```
