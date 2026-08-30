"""Cycle-count bar charts from a sweep CSV.

    python3 scripts/plot_cycles.py [sweep.csv] [out_dir]
"""
import csv
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

CSV = Path(sys.argv[1] if len(sys.argv) > 1 else "docs/private/sweep.csv")
OUT = Path(sys.argv[2] if len(sys.argv) > 2 else "docs/img")

CORE = "#C44E52"
ACCEL = "#55A868"

rows = list(csv.DictReader(CSV.open()))
shapes = sorted({r["shape"] for r in rows}, key=lambda s: int(s[1:]))
n_img = len({r["img"] for r in rows})

m = defaultdict(dict)
for s in shapes:
    r = [x for x in rows if x["shape"] == s]
    for k in ("core", "encode", "eval", "accel", "busy", "stalled"):
        m[s][k] = float(np.mean([int(x[k]) for x in r]))

OUT.mkdir(parents=True, exist_ok=True)
x = np.arange(len(shapes))
w = 0.38


def chart(left, right, left_label, right_label, title, path):
    """Two bars per shape with the ratio annotated above the right one."""
    fig, ax = plt.subplots(figsize=(6.0, 4.0))
    ax.bar(x - w / 2, left, w, color=CORE, label=left_label)
    ax.bar(x + w / 2, right, w, color=ACCEL, label=right_label)

    pad = max(left) * 0.02
    for i in range(len(shapes)):
        ax.text(i - w / 2, left[i] + pad, f"{left[i]:,.0f}",
                ha="center", va="bottom", fontsize=8)
        ax.text(i + w / 2, right[i] + pad, f"{right[i]:,.0f}",
                ha="center", va="bottom", fontsize=8)

        ax.text(i - w / 2, left[i] / 2, f"{left[i] / right[i]:.1f}x",
                ha="center", va="center", fontsize=11, color="white",
                fontweight="bold")

    ax.set_xticks(x)
    ax.set_xticklabels(shapes)
    ax.set_xlabel("hidden layer size")
    ax.set_ylabel("cycles")
    ax.set_ylim(0, max(left) * 1.18)
    ax.set_title(title, fontsize=10, pad=14)
    ax.legend(fontsize=8)
    ax.spines[["top", "right"]].set_visible(False)
    fig.tight_layout()
    fig.savefig(path, dpi=150)


core = np.array([m[s]["core"] for s in shapes])
accel = np.array([m[s]["accel"] for s in shapes])
host_eval = np.array([m[s]["eval"] for s in shapes])
busy = np.array([m[s]["busy"] for s in shapes])

chart(core, accel, "core alone", "core + accelerator",
      f"End to end",
      OUT / "cycles_end_to_end.png")

chart(host_eval, busy, "core", "accelerator (busy cycles)",
      "Network evaluation only: the work the accelerator replaces",
      OUT / "cycles_eval.png")

# --- per image, one shape
# PER_IMAGE_SHAPE = "h128"
#
# per = sorted((r for r in rows if r["shape"] == PER_IMAGE_SHAPE),
#              key=lambda r: int(r["img"]))
# imgs = [int(r["img"]) for r in per]
# enc = np.array([int(r["encode"]) for r in per])
# ev = np.array([int(r["eval"]) for r in per])
# bz = np.array([int(r["busy"]) for r in per])
#
# xi = np.arange(len(per))
# bw = 0.27
# fig, ax = plt.subplots(figsize=(8.5, 4.2))
# ax.bar(xi - bw, enc, bw, color="#4C72B0", label="encode: core")
# ax.bar(xi, ev, bw, color=CORE, label="network evaluation: core")
# ax.bar(xi + bw, bz, bw, color=ACCEL, label="network evaluation: accelerator")
#
# ax.set_xticks(xi)
# ax.set_xticklabels(imgs)
# ax.set_xlabel("MNIST test image")
# ax.set_ylabel("cycles")
# ax.set_ylim(0, max(ev.max(), enc.max()) * 1.12)
# ax.set_title(f"Per image, {PER_IMAGE_SHAPE}: encode is flat, evaluation "
#              f"tracks the spike count", fontsize=10, pad=14)
# ax.legend(fontsize=8)
# ax.spines[["top", "right"]].set_visible(False)
# fig.tight_layout()
# fig.savefig(OUT / "cycles_per_image.png", dpi=150)

for s in shapes:
    print(f"{s:>5}  core {m[s]['core']:>10,.0f}  encode {m[s]['encode']:>9,.0f}  "
          f"eval {m[s]['eval']:>10,.0f}  "
          f"eval share {100 * m[s]['eval'] / m[s]['core']:.1f}%")
print(f"wrote {OUT}/cycles_end_to_end.png and {OUT}/cycles_eval.png")
