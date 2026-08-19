"""Export golden-model traces as RTL test vectors (`make vectors`), output goes to verif/vectors/<model>/

Index order is image-major:  idx = ((image * T) + t) * WIDTH + neuron
so a unit TB reach it quickly. manifest.json carries the shapes, the config, and a SHA-256 of the weight files so a vector set can never be silently paired with the wrong weights.

What each file is for:
    s_in     [N,T,784]  input spikes            drives the spike queue
    ev_idx   [flat]     firing input indices    the event queue's actual input
    ev_len   [N,T]      events per timestep     how far to read into ev_idx
    acc1     [N,T,128]  drained weight sums     
    v1       [N,T,128]  membrane after update   
    spk1     [N,T,128]  hidden spikes           layer 2's input
    acc2     [N,T,10]
    v2       [N,T,10]
    spk2     [N,T,10]
    counts   [N,10]     output spike totals     argmax input
    pred     [N]        golden classification
    labels   [N]        MNIST ground truth
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from golden import forward, load_mnist_test, load_run 


def _hexfmt(arr, digits):
    flat = np.asarray(arr).ravel()
    mask = (1 << (4 * digits)) - 1
    return [f"{int(v) & mask:0{digits}x}" for v in flat]


def write_hex(path, arr, digits, comment):
    lines = [f"// {comment}", f"// {arr.shape} -> {arr.size} values, {digits} hex digits"]
    lines += _hexfmt(arr, digits)
    path.write_text("\n".join(lines) + "\n")
    return arr.shape


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()[:16]


def stack_trace(trace, key):
    return np.stack([step[key] for step in trace]).transpose(1, 0, 2)


def event_lists(s_in):
    """Convert [N,T,P] spike data into a flat event-index queue and per-timestep spike counts.
    """
    N, T, P = s_in.shape
    lens = s_in.sum(axis=2).astype(np.int32)
    idx = np.concatenate([np.nonzero(s_in[i, t])[0] for i in range(N) for t in range(T)])
    return idx.astype(np.int32), lens


def pack_weights(w, words_per_src, lanes):
    """Reshape a [dst, src] int8 weight matrix into the weight memory's layout and the one written in drain.v RTL. (weight between src neuron and dst neuron)

    For a w1 example, when looping through ev_idx each value represents a firing source neuron, the accumulation goes through every hidden layer neuron. So the weight layout in transposed [src,dst] order allows them to be encoded into words for each source neuron, which goes through 32 cycles(4 weights per word, 32 words in total for 128 hidden neurons). 
    """
    n_dst, n_src = w.shape
    slots = words_per_src * lanes #128/16
    padded = np.zeros((n_src, slots), dtype=np.int64)
    padded[:, :n_dst] = w.T # [src, dst], zero-padded for layer 2 10-15 slots

    b = padded.astype(np.int64) & 0xFF # encode each signed weight as an 8-bit pattern
    words = np.zeros((n_src, words_per_src), dtype=np.uint32)
    for j in range(lanes):
        words |= (b[:, j::lanes].astype(np.uint32) << (8 * j))
    return words.reshape(-1) #flattens to w for source0, w for source1, w for source2, etc.


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", default="model/training/runs/snn_h128_k2_T20")
    ap.add_argument("--n", type=int, default=10, help="#images to trace")
    ap.add_argument("--start", type=int, default=0, help="first test-set index")
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--out", default="verif/vectors")
    args = ap.parse_args()

    run_dir = Path(args.run)
    w1, w2, cfg = load_run(run_dir)
    images, labels = load_mnist_test()

    idx = np.arange(args.start, args.start + args.n)
    counts, trace = forward(images[idx], idx, w1, w2, cfg, args.seed, record=True)
    pred = counts.argmax(axis=1).astype(np.int32)

    out = Path(args.out) / run_dir.name
    out.mkdir(parents=True, exist_ok=True)

    s_in = stack_trace(trace, "s_in").astype(np.int32)
    ev_idx, ev_len = event_lists(s_in)

    files = {}
    def emit(name, arr, digits, comment):
        files[name] = {"file": f"{name}.hex", "shape": list(arr.shape),
                       "digits": digits, "note": comment}
        write_hex(out / f"{name}.hex", arr, digits, comment)

    emit("s_in",   s_in,                          1, "input spikes, 0/1")
    emit("ev_idx", ev_idx,                        3, "firing input indices, concatenated") #256<784<4096
    emit("ev_len", ev_len,                        3, "events per (image,timestep)") #max 784
    emit("acc1",   stack_trace(trace, "acc1"),    8, "fc1 drained weight sums, int32")
    emit("v1",     stack_trace(trace, "v1"),      4, "fc1 membrane after update, int16")
    emit("spk1",   stack_trace(trace, "spk1"),    1, "fc1 spikes, 0/1")
    emit("acc2",   stack_trace(trace, "acc2"),    8, "fc2 drained weight sums, int32")
    emit("v2",     stack_trace(trace, "v2"),      4, "fc2 membrane after update, int16")
    emit("spk2",   stack_trace(trace, "spk2"),    1, "fc2 spikes, 0/1")
    emit("counts", counts,                        2, "output spike totals over T") #max T=20
    emit("pred",   pred,                          1, "golden argmax classification")
    emit("labels", labels[idx].astype(np.int32),  1, "MNIST ground truth")
    emit("images", images[idx], 2, "raw MNIST pixels, uint8")

    lanes = 4
    w1_words = cfg["hidden_size"] // lanes
    w2_words = 4
    emit("w1", pack_weights(w1, w1_words, lanes), 8,
         f"w1 image: {w1_words} words per input, {lanes} int8 per word")
    emit("w2", pack_weights(w2, w2_words, lanes), 8,
         f"w2 image: {w2_words} words per hidden neuron, padded from "
         f"{-(-cfg['output_size'] // lanes)}")

    manifest = {
        "run": str(run_dir),
        "seed": args.seed,
        "n_images": int(args.n),
        "image_indices": [int(i) for i in idx],
        "index_order": "((image * T) + t) * width + neuron",
        "T": cfg["T"], "k": cfg["k"],
        "v_th_int": cfg["v_th_int"],
        "sizes": {"input": cfg["input_size"], "hidden": cfg["hidden_size"],
                  "output": cfg["output_size"]},
        "weights_sha256": {"fc1": sha256(run_dir / "weights_fc1.npy"),
                           "fc2": sha256(run_dir / "weights_fc2.npy")},
        "files": files,
    }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")

    correct = int((pred == labels[idx]).sum())
    print(f"wrote {len(files) + 1} files to {out}/")
    print(f"  images {idx[0]}..{idx[-1]}   golden correct {correct}/{args.n}")
    print(f"  input events: {ev_len.sum()} total, {ev_len.sum() / args.n:.0f} per image")
    print(f"  pred   {pred.tolist()}")
    print(f"  labels {labels[idx].tolist()}")


if __name__ == "__main__":
    main()
