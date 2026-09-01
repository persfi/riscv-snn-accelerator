# RISC-V CPU core + Event-Driven SNN Accelerator

RV32I core driving a spiking-neural-network accelerator. Fixed dataflow and memory layout with no arbiters, but cycle count is data-dependent.

RTL is intentionally a minority of this repo. Most of the code is verification: a Verilator C++ harness and a testbench per block, under `/verif`.

Started July 2026.

<br>

## Overview

This project builds an RV32I CPU and an event-driven SNN accelerator from scratch, together with the network they run and the software that drives them.

- **Core.** The core is a single-cycle RV32I implementation, passed the official rv32ui suite.
- **Model.** The network is a two-layer LIF SNN, trained on MNIST with snntorch. Reaches 97.6% test accuracy, and its integer form creates the golden reference model.
- **Accelerator.** The accelerator is a custom event-driven datapath, built from scratch and run on a fixed dataflow with no dynamic arbitration. It evaluates the network in 28x fewer cycles than the core, and every value it produces matches the reference model exactly.
- **Software.** A bare-metal C runtime runs on the core and drives the accelerator.
- **Verification.** 23 RTL blocks have their own C++ testbench. The core is then checked against the official RISC-V test suite, the accelerator against the golden reference model.

<br>

## Results

The same accelerator RTL can run 3 different networks (hidden layer size of 32,64,128) of the LIF SNN on the MNIST dataset, where the hidden layer size is written to the accelerator at runtime. 

The main metric to evaluate the performance of the accelerator design is its cycle counts for inference. All values below are produced by taking the mean of the results from 0-9 MNIST test images.

<br>

**End to end**,  whole image in to prediction out. (include image encoding)

| hidden | core alone | core + accelerator | speedup | model accuracy (integer) |
|---|---|---|---|---|
| 32 | 852,602 | 403,483 | 2.1x | 95.74% |
| 64 | 1,278,066 | 404,258 | 3.2x | 96.95% |
| 128 | 2,120,792 | 405,776 | 5.2x | 97.63% |

**Network evaluation only**, the architecture actually replaced by the accelerator. (exclude image encoding)

| hidden | core | accelerator busy | accelerator busy% | speedup |
|---|---|---|---|---|
| 32 | 446,652 | 15,453 | 4.0% | 28.9x |
| 64 | 871,712 | 30,583 | 8.0% | 28.5x |
| 128 | 1,713,671 | 60,457 | 15.7% | 28.3x |

<br>

Counting only the cycles where something is working, the host encoder takes 96.3% of them at h32 and 86.9% at h128. The bottleneck is the host encoding, not the accelerator architecture.

<p>
<img src="docs/img/cycles_end_to_end.png" width="49%" alt="End to end cycle counts">
<img src="docs/img/cycles_eval.png" width="49%" alt="Network evaluation cycle counts">
</p>
<br>

## Verification

The golden reference model in `model/golden/` defines the correct answer for everything the accelerator does. It computes in integers with the same arithmetic shifts and saturation bounds as the RTL, so every value it produces matches the hardware exactly. Its outputs become the vectors every testbench compares against. Full tables and negative controls in [DESIGN.md §4](DESIGN.md#4-verification).

| | result |
|---|---|
| Official rv32ui tests | 40 / 40 pass, 2 excluded (ma_data, fence_i: reasons in DESIGN.md) |
| Unit testbenches | 23 blocks, 121,131 checks, golden reference model checks included in accel and sequencer tb |
| System-level | 3 shapes × 10 images, 300 class counts and 30 argmaxes compared, 0 mismatches |
| Spike encoder | 18,030 firing indices compared C vs NumPy, 0 mismatches |

The rv32ui, unit testbench and spike encoder results each have a negative control: a deliberate break that made the check fail, then reverted.

<br>

## Architecture at a glance

No arbiters and no dynamic scheduling anywhere in the datapath: every cycle has exactly one possible next action, so the cycle count is a function of the input data alone.

<br>

## Build & run

Requires Verilator 5.032, `riscv64-unknown-elf-gcc` 14.2.0, GNU Make 4.4, and a Python venv from `requirements.txt` (numpy, torch, snntorch, matplotlib). Exact versions in [docs/tool_versions.md](docs/tool_versions.md).

Quickstart:

```bash
git clone --recursive https://github.com/persfi/riscv-snn-accelerator
cd riscv-snn-accelerator
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
make check
```

If you already cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

Make commands:
| target | what it does | scope |
|---|---|---|
| `make check` | every unit testbench, then lint, then riscv-tests | 23 blocks, accelerator over all 3 networks, 40 rv32ui tests |
| `make lint` | `verilator --lint-only -Wall` | every file in `rtl/` |
| `make test-core` | official rv32ui suite, pass/fail per test | 40 tests, no network |
| `make u-<block>` | one unit testbench, e.g. `make u-accel`, `make u-lif_unit` | one block|
| `make test-system` | compiled C on the core driving the accelerator, vs the golden model | 10 images, one network |
| `make test-encode` | on-core C spike encoder vs the golden model's encoder | 10 images, one network |
| `make bench` | core-only inference, for the cycle-count comparison | one image, one network |
| `make app-<name>` | build and run one `sw/apps/` program on the SoC | one program, one network |
| `make vectors` | regenerate `verif/vectors/` from the golden model | 10 images, one network |
| `make netcfg` | compile a trained run into `sw/libsnn/netcfg.h` | one network, no images |

Flags:

- `shape=h32`, `shape=h64`: run a different network. Works on any target, default is `h128`.
- `img=<0-9>`: choose which MNIST test image `bench` and `app-<name>` run. This rewrites `sw/libsnn/image.h` and leaves it rewritten.
- `TRACE=1`: prints a testbench's internal signals as it runs. `accel`, `sequencer` and `core` print a line every cycle.
<br>

## Repo layout

| path | contents |
|---|---|
| `rtl/core/` | RV32I single-cycle core |
| `rtl/accel/` | SNN accelerator |
| `rtl/soc/` | top level: core + accelerator + dmem, address decode |
| `verif/harness/` | shared C++ harness, riscv-tests runner, SoC app runner |
| `verif/unit/` | 23 per-block testbenches |
| `verif/vectors/` | golden-model output, per network shape |
| `verif/env/` | CSR-free environment for the official riscv-tests |
| `model/golden/` | golden reference SNN model in NumPy, same arithmetic as the RTL |
| `model/training/` | snntorch training and quantization |
| `tools/snnc/` | compiles a trained run into driver config |
| `sw/bsp/` | linker script, crt0, mmio.h |
| `sw/libsnn/` | on-core runtime: spike encoder and accelerator driver |
| `sw/apps/` | bare-metal programs: mnist, core-only mnist, encoder check, small core tests |
| `scripts/` | image hex to C array, cycle and distribution plots |

<br>

## Where to look

| | |
|---|---|
| [DESIGN.md](DESIGN.md) | design rationale, memory map, decisions, verification, and results |
| [DESIGN.md §2](DESIGN.md#2-system-architecture) | memory map, register table, host/accelerator contract |
| [DESIGN.md §3](DESIGN.md#3-decisions--rejected-alternatives) | every design decision, what was rejected and why |
| [DESIGN.md §4](DESIGN.md#4-verification) | full verification results and negative controls |
| [DESIGN.md §5](DESIGN.md#5-results) | acceleration results: cycle counts |
| [docs/LOG.md](docs/LOG.md) | daily engineering log |

<br>

## What's next

- **FPGA deployment on the Arty A7-100T.** 
- **The spike encoder moves into the accelerator.** 

