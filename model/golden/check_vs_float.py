"""Validate the integer golden model against the trained float model.

Both get the SAME spike train (rate_code.py). There are small differences, so equal
classifications have to be verified within a margin:

    1. weights    float uses the int8 grid scaled back to float; int uses the
                  int8 integers that's scaled then rounded.
    2. threshold  float fires at 1.0; int's threshold is scaled and rounded.
    3. leak       float multiplies by 0.75; int does v -= v>>2, which floors drops the low bits, so small v (0-3) does not leak at all.

Accumulation depends on (1) and (2), and saturation is never fired.

The none-tie inference difference should be less than 1%: exit status is 0 on PASS, 1 on FAIL.
"""
import argparse
import sys
from pathlib import Path

import numpy as np
import torch

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "training"))

from golden import drain, load_mnist_test, load_run  # noqa: E402
from rate_code import spike_train  # noqa: E402


@torch.no_grad()
def float_forward(model, spikes_t):
    """RateSNN.forward, but fed an externally supplied spike train.
    """
    mem1 = model.lif1.reset_mem()
    mem2 = model.lif2.reset_mem()
    out = torch.zeros(spikes_t.shape[1], model.fc2.out_features)
    for t in range(model.T):
        spk1, mem1 = model.lif1(model.fc1(spikes_t[t]), mem1)
        spk2, mem2 = model.lif2(model.fc2(spk1), mem2)
        out = out + spk2
    return out.numpy()


def int_forward(spikes, w1, w2, cfg):
    """The golden integer path, driven by a precomputed [T,N,P] spike train."""
    from lif import lif_update

    T, k = cfg["T"], cfg["k"]
    vth1, vth2 = cfg["v_th_int"]["fc1"], cfg["v_th_int"]["fc2"]
    N, H, O = spikes.shape[1], w1.shape[0], w2.shape[0]
    w1f, w2f = w1.astype(np.float64), w2.astype(np.float64)

    v1 = np.zeros((N, H), dtype=np.int16)
    v2 = np.zeros((N, O), dtype=np.int16)
    out = np.zeros((N, O), dtype=np.int32)
    for t in range(T):
        v1, spk1 = lif_update(v1, drain(w1f, spikes[t]), vth1, k)
        v2, spk2 = lif_update(v2, drain(w2f, spk1.astype(np.int32)), vth2, k)
        out += spk2
    return out


def load_float_model(run_dir, cfg):
    from model import RateSNN

    net = RateSNN(cfg["input_size"], cfg["hidden_size"], cfg["output_size"],
                  k=cfg["k"], T=cfg["T"], v_th=cfg["v_th_float"])
    net.load_state_dict(torch.load(Path(run_dir) / "model.pt", map_location="cpu"))
    net.eval()
    return net


def is_tie(counts, pred):
    """True if some other class matched the winner's spike count. Counts are small integers, so ties are common.
    """
    return int((counts == counts[pred]).sum()) > 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", default="model/training/runs/snn_h128_k2_T20")
    ap.add_argument("--n", type=int, default=None, help="#test images")
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--batch", type=int, default=500, help="memory chunk only")
    ap.add_argument("--max-report", type=int, default=20)
    ap.add_argument("--min-agreement", type=float, default=0.99,
                    help="pass threshold on non-tie agreement; exact agreement is "
                         "not reachable while the int leak is a shift and the "
                         "float leak is a multiply")
    args = ap.parse_args()

    w1, w2, cfg = load_run(args.run)
    net = load_float_model(args.run, cfg)
    images, labels = load_mnist_test()
    images, labels = images[:args.n], labels[:args.n]

    mismatches, ties = [], 0
    f_correct = i_correct = 0

    for lo in range(0, len(images), args.batch):
        hi = min(lo + args.batch, len(images))
        idx = np.arange(lo, hi)
        spikes = spike_train(images[idx], cfg["T"], idx, args.seed)

        f_out = float_forward(net, torch.from_numpy(spikes).float())
        i_out = int_forward(spikes, w1, w2, cfg)
        f_pred, i_pred = f_out.argmax(axis=1), i_out.argmax(axis=1)

        f_correct += int((f_pred == labels[idx]).sum())
        i_correct += int((i_pred == labels[idx]).sum())

        for j in np.nonzero(f_pred != i_pred)[0]:
            tied = is_tie(f_out[j], f_pred[j]) or is_tie(i_out[j], i_pred[j])
            ties += tied
            mismatches.append((int(idx[j]), int(labels[idx][j]), int(f_pred[j]),
                               int(i_pred[j]), f_out[j], i_out[j], tied))

    n = len(images)
    real = [m for m in mismatches if not m[6]]

    print(f"run   {args.run}")
    print(f"n={n}  seed={args.seed}  T={cfg['T']}  k={cfg['k']}  "
          f"v_th={cfg['v_th_int']['fc1']}/{cfg['v_th_int']['fc2']}\n")
    print(f"float accuracy      {f_correct / n:.4f}")
    print(f"int   accuracy      {i_correct / n:.4f}   ({i_correct - f_correct:+d} images)")
    print(f"argmax agreement    {(n - len(mismatches)) / n:.4f}   "
          f"({len(mismatches)} differ: {len(real)} real, {ties} tie-breaks)\n")

    if mismatches:
        print(f"{'image':>6} {'label':>5} {'float':>5} {'int':>5}  {'tie':>3}  "
              f"float counts / int counts")
        for img, lab, fp, ip, fc, ic, tied in mismatches[:args.max_report]:
            print(f"{img:>6} {lab:>5} {fp:>5} {ip:>5}  {'y' if tied else ' ':>3}  "
                  f"{np.round(fc).astype(int).tolist()}")
            print(f"{'':>29}{ic.tolist()}")
        if len(mismatches) > args.max_report:
            print(f"  ... {len(mismatches) - args.max_report} more")
        print()

    agreement = (n - len(real)) / n
    verdict = "PASS" if agreement >= args.min_agreement else "FAIL"
    print(f"{verdict}  non-tie agreement {agreement:.4f} "
          f"(threshold {args.min_agreement:.4f}), {len(real)}/{n} real mismatch(es)"
          + (f", {ties} tie-break(s) ignored" if ties else ""))
    return 0 if verdict == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
