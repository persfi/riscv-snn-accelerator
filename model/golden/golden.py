"""Golden model: bit-exact integer forward of the LIF SNN (the correctness oracle).
"""
import json
from pathlib import Path

import numpy as np

from rate_code import spikes_at

INT16_MIN, INT16_MAX = -32768, 32767


def load_run(run_dir):
    """Load a frozen training export: int8 weights + config (k, T, V_th_int, sizes)."""
    run_dir = Path(run_dir)
    cfg = json.loads((run_dir / "config.json").read_text())
    w1 = np.load(run_dir / "weights_fc1.npy").astype(np.int32)  # [hidden, input]
    w2 = np.load(run_dir / "weights_fc2.npy").astype(np.int32)  # [output, hidden]
    return w1, w2, cfg

def drain(weights_f, spikes):
    return np.rint(spikes.astype(np.float64) @ weights_f.T).astype(np.int32)


def forward(images, img_idx, w1, w2, cfg, seed=0, record=False):
    from lif import lif_update

    T, k = cfg["T"], cfg["k"]
    vth1, vth2 = cfg["v_th_int"]["fc1"], cfg["v_th_int"]["fc2"]
    N, H, O = images.shape[0], w1.shape[0], w2.shape[0]
    w1f, w2f = w1.astype(np.float64), w2.astype(np.float64)   # cast weights once

    v1 = np.zeros((N, H), dtype=np.int16)   # membranes reset once per inference (per image)
    v2 = np.zeros((N, O), dtype=np.int16)
    out_count = np.zeros((N, O), dtype=np.int32)
    trace = [] if record else None

    for t in range(T):
        s_in = spikes_at(images, t, img_idx, seed)   # deterministic per (image, t, pixel)
        acc1 = drain(w1f, s_in)                          
        v1, spk1 = lif_update(v1, acc1, vth1, k)        
        acc2 = drain(w2f, spk1.astype(np.int32))        
        v2, spk2 = lif_update(v2, acc2, vth2, k)         
        out_count += spk2                      

        if record:
            trace.append(dict(t=t, s_in=s_in, acc1=acc1, v1=v1.copy(), spk1=spk1,
                              acc2=acc2, v2=v2.copy(), spk2=spk2))
    return out_count, trace


def load_mnist_test(root="model/training/.data"):
    from torchvision import datasets
    ds = datasets.MNIST(root, train=False, download=False)
    return ds.data.numpy().reshape(len(ds), -1), ds.targets.numpy()


def accuracy(run_dir, n=None, seed=0, batch=2000):
    w1, w2, cfg = load_run(run_dir)
    images, labels = load_mnist_test()
    if n is not None:
        images, labels = images[:n], labels[:n]

    correct = 0
    for i in range(0, len(images), batch):
        idx = np.arange(i, min(i + batch, len(images)))
        out, _ = forward(images[idx], idx, w1, w2, cfg, seed)
        correct += int((out.argmax(axis=1) == labels[idx]).sum())
    return correct / len(images)


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", default="model/training/runs/snn_h128_k2_T20")
    ap.add_argument("--n", type=int, default=None, help="limit #test images (default all 10k)")
    ap.add_argument("--seed", type=int, default=0) # seed for rate coding
    ap.add_argument("--check-invariance", action="store_true",
                    help="assert the batch knob does not move the answer")
    args = ap.parse_args()

    if args.check_invariance:
        n = args.n or 1000
        accs = {b: accuracy(args.run, n=n, seed=args.seed, batch=b)
                for b in (97, 500, 2000)}
        for b, a in accs.items():
            print(f"  batch={b:5d}  acc(n={n}) = {a:.4f}")
        assert len(set(accs.values())) == 1, "batch size changed the oracle's answer"
        print("batch invariance: ok")

    acc = accuracy(args.run, n=args.n, seed=args.seed)
    print(f"golden integer test accuracy ({args.run}): {acc:.4f}")
