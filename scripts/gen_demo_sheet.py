#!/usr/bin/env python3
"""Render the vector-set images as a demo reference sheet.
"""

import argparse
import base64
import os
import struct
import zlib


def read_hex(path):
    """Values from one of the exported .hex files, comments stripped."""
    out = []
    with open(path) as f:
        for line in f:
            line = line.split("//")[0].strip()
            if line:
                out.append(int(line, 16))
    return out


def png_gray(pixels, w, h):
    """Minimal 8-bit grayscale PNG. pixels is a flat list of 0-255."""
    raw = b"".join(
        b"\x00" + bytes(pixels[y * w:(y + 1) * w]) for y in range(h)
    )

    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c))

    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 0, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw, 9))
        + chunk(b"IEND", b"")
    )


CELL = """
  <figure>
    <img src="data:image/png;base64,{png}" alt="MNIST digit {label}">
    <figcaption>
      <span class="idx">image {idx}</span>
      <span class="row"><b>switches</b> <code>{sw}</code></span>
      <span class="row"><b>LEDs</b> <code>{leds}</code></span>
      <span class="row {cls}"><b>predicts</b> {pred} &middot; label {label}</span>
    </figcaption>
  </figure>"""

PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Demo Sheet</title>
<style>
  body {{ font: 14px/1.5 system-ui, sans-serif; margin: 2rem; color: #111; }}
  h1 {{ font-size: 1.2rem; margin: 0 0 .2rem; }}
  p.sub {{ margin: 0 0 1.5rem; color: #555; }}
  .grid {{ display: flex; flex-wrap: wrap; gap: 1.25rem; }}
  figure {{ margin: 0; border: 1px solid #ccc; padding: .6rem; border-radius: 4px; }}
  img {{ width: 112px; height: 112px; image-rendering: pixelated; display: block;
         background: #000; }}
  figcaption {{ margin-top: .5rem; font-size: 12px; }}
  figcaption span {{ display: block; }}
  .idx {{ font-weight: 600; margin-bottom: .2rem; }}
  code {{ font-size: 12px; background: #f2f2f2; padding: 0 .25rem; }}
  .miss {{ color: #b00; }}
  @media print {{ body {{ margin: .5rem; }} figure {{ break-inside: avoid; }} }}
</style>
</head>
<body>
<h1>{run}</h1>
<p class="sub">Switch order is sw[3] sw[2] sw[1] sw[0]; LED order is led[3] led[2] led[1] led[0].
These results are from the golden model, used to cross reference with the output of the board.</p>
<div class="grid">{cells}
</div>
</body>
</html>
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--run", default="verif/vectors/snn_h128_k2_T20")
    ap.add_argument("--out", default="verif/build/demo_sheet.html")
    args = ap.parse_args()

    pixels = read_hex(os.path.join(args.run, "images.hex"))
    preds = read_hex(os.path.join(args.run, "pred.hex")) #from golden model predictions not the mnist label
    labels = read_hex(os.path.join(args.run, "labels.hex"))

    n = len(preds)
    if len(pixels) != n * 784:
        raise SystemExit(
            f"{len(pixels)} pixels for {n} images: expected {n * 784}"
        )

    cells = []
    for i in range(n):
        png = png_gray(pixels[i * 784:(i + 1) * 784], 28, 28)
        cells.append(CELL.format(
            png=base64.b64encode(png).decode(),
            idx=i,
            sw=format(i, "04b"),
            leds=format(preds[i], "04b"),
            pred=preds[i],
            label=labels[i],
            cls="" if preds[i] == labels[i] else "miss",
        ))

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w") as f:
        f.write(PAGE.format(run=os.path.basename(args.run), cells="".join(cells)))
    print(f"{args.out}: {n} images")


if __name__ == "__main__":
    main()
