#!/usr/bin/env python3
"""
Usage:  python3 lif_probe.py      (needs snntorch + torch, see requirements.txt)
"""

import inspect
import torch
import snntorch as snn


def beta_from_k(k: int) -> float:
    # lif_spec : leak `V -= V>>k`  <=>  `V *= (1 - 2^-k)`  =>  beta = 1 - 2^-k
    return 1.0 - 2.0 ** (-k)


def leaky_supports_reset_delay() -> bool:
    return "reset_delay" in inspect.signature(snn.Leaky.__init__).parameters


def run_trace(inputs, k, threshold, reset_mechanism="subtract", reset_delay=None):
    
    beta = beta_from_k(k)

    kwargs = dict(
        beta=beta,
        threshold=float(threshold),
        reset_mechanism=reset_mechanism,
        init_hidden=False,   # passes mem explicitly
    )
    label_delay = "n/a"
    if reset_delay is not None:
        if leaky_supports_reset_delay():
            kwargs["reset_delay"] = reset_delay
            label_delay = str(reset_delay)
        else:
            label_delay = "UNSUPPORTED (this snntorch version has no reset_delay)"

    lif = snn.Leaky(**kwargs)

    mem = torch.zeros(1)          # V[0] = 0
    rows = []
    for t, x in enumerate(inputs, start=1):
        spk, mem = lif(torch.tensor([float(x)]), mem)
        rows.append((t, float(x), float(mem), int(spk.item())))

    print(f"  beta = 1 - 2^-{k} = {beta:g}    threshold = {threshold}    "
          f"reset = {reset_mechanism}    reset_delay = {label_delay}")
    print(f"  {'t':>2}  {'input':>8}  {'mem (after step)':>17}  {'spk':>4}")
    for t, x, m, s in rows:
        print(f"  {t:>2}  {x:>8.3f}  {m:>17.6f}  {s:>4}")
    return rows


def main():
    print("snntorch version        :", snn.__version__)
    print("Leaky has reset_delay   :", leaky_supports_reset_delay())
    print()

    print("== Experiment A: input added before or after leak? ==")
    run_trace(inputs=[40, 0, 5, 0], k=2, threshold=10_000)
    print()

   
    print("== Experiment B: reset on the firing step, or the next? ==")
    print("- reset_delay=False (matches design choice):")
    run_trace(inputs=[30, 0, 0], k=2, threshold=20, reset_delay=False)
    print()
    print("- reset_delay=True (deferred -- snntorch default):")
    run_trace(inputs=[30, 0, 0], k=2, threshold=20, reset_delay=True)
    print()

if __name__ == "__main__":
    main()
