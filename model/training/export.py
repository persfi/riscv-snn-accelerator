""""Freeze a trained RateSNN to integer artifacts: int8 weights + per-layer scale/threshold config.
"""
import json
from pathlib import Path

import numpy as np

from quant import quantize_int8


def export_model(model, run_dir, k, T, v_th):
    """Write weights_{fc1,fc2}.npy (int8) + config.json into run_dir. Returns config."""
    run_dir = Path(run_dir)
    run_dir.mkdir(parents=True, exist_ok=True)

    scales, v_th_int = {}, {}
    for name, layer in (("fc1", model.fc1), ("fc2", model.fc2)):
        q, scale = quantize_int8(layer.weight)
        np.save(run_dir / f"weights_{name}.npy", q.cpu().numpy())
        scales[name] = scale
        v_th_int[name] = round(v_th / scale)   # threshold into the layer's int domain

    config = {
        "input_size": model.fc1.in_features,
        "hidden_size": model.fc1.out_features,
        "output_size": model.fc2.out_features,
        "k": k,
        "T": T,
        "v_th_float": v_th,
        "scales": scales,       # float, offline metadata (not loaded into hardware)
        "v_th_int": v_th_int,   # per-layer integer threshold the hardware uses
    }
    with open(run_dir / "config.json", "w") as f:
        json.dump(config, f, indent=2)
    return config
