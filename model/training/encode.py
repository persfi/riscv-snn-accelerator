"""Rate encoding: pixel value -> Bernoulli spike train over T timesteps.
"""
import torch


def rate_encode(images, T, generator=None):
    """images: [batch, features] in 0~1. Returns [T, batch, features] in 0|1.
    Output is a float tensor so it feeds straight into the Linear layers.
    """
    if not (images.min() >= 0.0 and images.max() <= 1.0):
        raise ValueError("rate_encode expects pixel probabilities in [0,1]")

    # [batch, features] -> [T, batch, features], add new dimension, expand to T timesteps(same probabilities), then sample Bernoulli.
    p = images.unsqueeze(0).expand(T, *images.shape)
    return torch.bernoulli(p, generator=generator)


if __name__ == "__main__":
    # Smoke test
    torch.manual_seed(0)
    pixels = torch.tensor([[0.0, 0.25, 0.5, 0.75, 1.0]])  # [1, 5]
    T = 20_000
    spikes = rate_encode(pixels, T)  # [T, 1, 5]
    rate = spikes.mean(dim=0).squeeze(0)  # empirical rate per pixel
    print(f"shape   {tuple(spikes.shape)}  (T, batch, features)")
    print(f"pixels  {pixels.squeeze(0).tolist()}")
    print(f"rate    {[round(r, 3) for r in rate.tolist()]}")
    assert torch.allclose(rate, pixels.squeeze(0), atol=0.02), "rate should track pixel value"
    print("ok")
