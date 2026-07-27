"""Rate encoder: the exact spike train the hardware has to reproduce.
Pixel 0 never spikes:`u < 0` is false for every u
"""
import numpy as np

U32 = np.uint32

_T_SHIFT, _IMG_SHIFT = 10, 15


def wang32(x):
    """Thomas Wang's 32-bit integer hash, multiply-free form.
    """
    x = np.asarray(x, dtype=U32)
    with np.errstate(over="ignore"):   # wraparound is the algorithm, not an error
        x = ~x + (x << 15)
        x ^= x >> 12
        x = x + (x << 2)
        x ^= x >> 4
        x = (x + (x << 3)) + (x << 11)
        x ^= x >> 16
    return x


def seed_key(seed):
    return wang32(U32(seed))


def spikes_at(images_u8, t, img_idx, seed=0):
    """Spikes for one timestep.
    images_u8 : [N, P] uint8   raw MNIST pixels, 0..255 (no /255 rescale)
    t         : int            timestep index
    img_idx   : [N] integer    global image indices
    returns   : [N, P] int32   0/1
    """
    key = seed_key(seed)
    pix = np.arange(images_u8.shape[1], dtype=U32)
    ctr = ((img_idx.astype(U32)[:, None] << _IMG_SHIFT)
           | (U32(t) << _T_SHIFT)
           | pix[None, :])
    u = wang32(key ^ ctr) & U32(0xFF)
    return (u < images_u8).astype(np.int32)


def spike_train(images_u8, T, img_idx, seed=0):
    return np.stack([spikes_at(images_u8, t, img_idx, seed) for t in range(T)])


if __name__ == "__main__":

    # 1. verifies that spike frequency matches pixel_value / 256 and that black pixels never spike.
    px = np.arange(256, dtype=np.uint8)[None, :]
    idx = np.zeros(1, dtype=U32)
    train = spike_train(np.repeat(px, 1, axis=0), 4000, idx, seed=0)
    rate = train.mean(axis=(0, 1))
    err = np.abs(rate - px[0] / 256.0)
    print(f"rate error vs pixel/256: max {err.max():.4f}  mean {err.mean():.4f}")
    assert err.max() < 0.03, "encoder is biased"
    assert train[:, 0, 0].sum() == 0, "pixel 0 must never spike"

    # 2. changing the processing batch size does not change the spike trains.
    imgs = np.random.default_rng(1).integers(0, 256, (8, 784), dtype=np.uint8)
    all_at_once = spike_train(imgs, 20, np.arange(8, dtype=U32))
    in_halves = np.concatenate(
        [spike_train(imgs[:3], 20, np.arange(0, 3, dtype=U32)),
         spike_train(imgs[3:], 20, np.arange(3, 8, dtype=U32))], axis=1)
    assert np.array_equal(all_at_once, in_halves), "chunking changed the spikes"
    print("chunk invariance: ok")

    # 3. adjacent counters must not correlate 
    flat = spike_train(np.full((64, 784), 128, np.uint8), 20,
                       np.arange(64, dtype=U32)).astype(np.float64)
    print(f"p=0.5 pixels: measured {flat.mean():.4f}")
    nb = np.corrcoef(flat[:, :, :-1].ravel(), flat[:, :, 1:].ravel())[0, 1]
    print(f"neighbour-pixel correlation: {nb:+.5f}")
    assert abs(nb) < 0.01, "adjacent pixels are correlated -- weak avalanche"

    # 4. different seeds give different trains
    a = spike_train(imgs, 20, np.arange(8, dtype=U32), seed=0)
    b = spike_train(imgs, 20, np.arange(8, dtype=U32), seed=1)
    print(f"seed 0 vs 1 disagreement: {(a != b).mean():.4f}")
    assert (a != b).mean() > 0.1
    print("ok")
