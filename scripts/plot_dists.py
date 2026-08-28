"""Weight and accumulator distributions + graph for one vector set.

    python3 scripts/plot_dists.py [vector_dir] [out_dir]
"""
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

VEC = Path(sys.argv[1] if len(sys.argv) > 1
           else "verif/vectors/snn_h128_k2_T20")
OUT = Path(sys.argv[2] if len(sys.argv) > 2 else "docs/img")


def read_hex(path, bits):
    """Read the hex vector file as signed integers of the given width."""
    vals = []
    for line in path.read_text().splitlines():
        line = line.split("//")[0]
        vals.extend(int(tok, 16) for tok in line.split())
    a = np.array(vals, dtype=np.int64)
    return np.where(a >= 1 << (bits - 1), a - (1 << bits), a)


def unpack_weights(path):
    """w hex is four int8 weights per 32-bit word, low byte first."""
    words = read_hex(path, 32).astype(np.int64) & 0xFFFFFFFF
    b = np.stack([(words >> s) & 0xFF for s in (0, 8, 16, 24)], axis=1).ravel()
    return np.where(b >= 128, b - 256, b)


w = np.concatenate([unpack_weights(VEC / "w1.hex"),
                    unpack_weights(VEC / "w2.hex")])
acc = np.concatenate([read_hex(VEC / "acc1.hex", 32),
                      read_hex(VEC / "acc2.hex", 32)])
v = np.concatenate([read_hex(VEC / "v1.hex", 16),
                    read_hex(VEC / "v2.hex", 16)])

print(f"{VEC}")
print(f"  weights   n={w.size}  min={w.min()}  max={w.max()}  "
      f"mean={w.mean():.2f}  std={w.std():.1f}")
print(f"            |w|>=64: {(np.abs(w) >= 64).mean() * 100:.2f}%   "
      f"|w|>=100: {(np.abs(w) >= 100).mean() * 100:.3f}%")
print(f"  acc       n={acc.size}  min={acc.min()}  max={acc.max()}  "
      f"max|acc|={np.abs(acc).max()}")
for p in (50, 99, 99.9, 100):
    print(f"            p{p:<5} |acc| = {np.percentile(np.abs(acc), p):.0f}")
print(f"  V         n={v.size}  min={v.min()}  max={v.max()}  "
      f"max|v|={np.abs(v).max()}")
print(f"  int16 headroom on max|v|: {32767 / np.abs(v).max():.1f}x")

OUT.mkdir(parents=True, exist_ok=True)
fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(15, 4))

ax1.hist(w, bins=np.arange(-128, 129, 4), color="#4C72B0")
ax1.set_title(f"int8 weights  (n={w.size})")
ax1.set_xlabel("weight value")
ax1.set_ylabel("count")
ax1.set_yscale("log")

ax2.hist(acc, bins=120, color="#C44E52")
ax2.set_title(f"acc, int32  (max|acc| = {np.abs(acc).max()})")
ax2.text(0.5, 0.88, f"int32 limit +/-2,147,483,647 is off scale\n"
                    f"({2147483647 // np.abs(acc).max():,}x the observed max)",
         transform=ax2.transAxes, ha="center", va="top", fontsize=8)
ax2.set_xlabel("acc value")
ax2.set_ylabel("count")
ax2.set_yscale("log")

ax3.hist(v, bins=120, color="#55A868")
ax3.axvline(32767, color="k", ls="--", lw=1, label="int16 limit")
ax3.axvline(-32768, color="k", ls="--", lw=1)
ax3.set_xlim(-35000, 35000)
ax3.set_title(f"V, int16  (max|v| = {np.abs(v).max()})")
ax3.set_xlabel("membrane potential")
ax3.set_ylabel("count")
ax3.set_yscale("log")
ax3.legend()

fig.suptitle(f"{VEC.name}: small signed weights keep acc bounded, and the leak "
             f"plus threshold keep V well inside int16")
fig.tight_layout()
path = OUT / f"dists_{VEC.name}.png"
fig.savefig(path, dpi=130)
print(f"  wrote {path}")
