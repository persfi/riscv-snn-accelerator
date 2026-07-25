"""MNIST loading for the rate-coded SNN.
"""
from pathlib import Path

import torch
from torch.utils.data import DataLoader, random_split
from torchvision import datasets, transforms

_TRAIN_SIZE = 50_000
_VAL_SIZE = 10_000  # 50k + 10k = the 60k MNIST training set


def _transform():
    # ToTensor(): uint8 [0,255] -> float [0,1], shape [1,28,28].
    # flatten to [784] for the fully-connected input layer.
    return transforms.Compose(
        [
            transforms.ToTensor(),
            transforms.Lambda(lambda x: x.view(-1)),
        ]
    )


def mnist_loaders(root="model/training/.data", batch_size=128, seed=0, num_workers=0):
    """Return (train_loader, val_loader, test_loader).
    Pixels arrive in [0,1] (spike probabilities). Downloads to `root` on first
    use. `num_workers=0` by default: mnist is small.
    """
    root = Path(root)
    tfm = _transform()

    full_train = datasets.MNIST(root, train=True, download=True, transform=tfm)
    test_set = datasets.MNIST(root, train=False, download=True, transform=tfm) 
    #download test or train from ds

    gen = torch.Generator().manual_seed(seed) #same seed ensures the same split
    train_set, val_set = random_split(
        full_train, [_TRAIN_SIZE, _VAL_SIZE], generator=gen
    ) # breaks full train into train and val 50000/10000

    def loader(ds, shuffle):
        return DataLoader(
            ds, batch_size=batch_size, shuffle=shuffle, num_workers=num_workers
        )

    return loader(train_set, True), loader(val_set, False), loader(test_set, False)
    # only train shuffle because gradient descent benefits from seeing samples in a new random order each epoch

#run this as main if this file is executed directly, not imported
if __name__ == "__main__": 
    # Smoke test: shapes, counts, and that pixels stayed in [0,1].
    tr, va, te = mnist_loaders(batch_size=64)
    xb, yb = next(iter(tr))
    print(f"batches  train/val/test = {len(tr)}/{len(va)}/{len(te)}")
    print(f"x batch  shape={tuple(xb.shape)}  min={xb.min():.3f}  max={xb.max():.3f}")
    print(f"y batch  shape={tuple(yb.shape)}  labels={yb[:8].tolist()}") 
    #first 8 labels(differs each run since train is shuffled)

    assert xb.shape[1] == 784, "expected flattened 784-pixel input"
    assert 0.0 <= xb.min() and xb.max() <= 1.0, "pixels must stay in [0,1] for rate coding"
    print("ok")

    #50000/64 = 781.25 -> 782 batches, 10000/64 = 156.25 -> 157 batches, 10000/64 = 156.25 -> 157 batches
