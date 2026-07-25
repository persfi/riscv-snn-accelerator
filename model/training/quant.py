"""Quantization-aware training for the int8 weights.
"""
import torch
import torch.nn as nn
import torch.nn.functional as F

_QMAX = 127  # symmetric int8: weights map into [-127, 127]


class _RoundSTE(torch.autograd.Function):

    @staticmethod
    def forward(ctx, x):
        return torch.round(x)

    @staticmethod
    def backward(ctx, grad_out):
        return grad_out  

def _round_ste(x): #input scaled weight(un-rounded)
    return _RoundSTE.apply(x)


def per_layer_scale(w):
    """Symmetric dynamic scale for a weight tensor: max|w| / 127.
    """
    return w.abs().max().clamp(min=1e-8) / _QMAX


def fake_quant(w):
    """Quantize a weight tensor to its int8 grid, but represented in float.
       Will be passed forward for prediction.
    """
    scale = per_layer_scale(w).detach()
    q = _round_ste(w / scale).clamp(-_QMAX, _QMAX)
    return q * scale


def quantize_int8(w):
    """Freeze to the actual int8 integers + scale, for export (no gradient).
    """
    with torch.no_grad():
        scale = per_layer_scale(w)
        q = torch.round(w / scale).clamp(-_QMAX, _QMAX).to(torch.int8)
    return q, float(scale)


class QuantLinear(nn.Linear):
    """Linear layer that fake-quantizes its weights to int8 on every forward pass.
    """

    def __init__(self, in_features, out_features):
        super().__init__(in_features, out_features, bias=False) #no constant current needed

    def forward(self, x):
        return F.linear(x, fake_quant(self.weight))


if __name__ == "__main__":
    # Smoke test: fake-quant error is sub-step, STE gradient is clean, small
    # weights still use the full int8 range.

    torch.manual_seed(0)
    w = (torch.randn(4, 5) * 0.1).requires_grad_(True)  
    # create 4x5 weight matrix that represents 5 inputs and 4 outputs, with small random values

    wq = fake_quant(w) #scaled, rounded weights, passed forward
    step = float(per_layer_scale(w).detach()) #scale
    print(f"max fake-quant error {float((w - wq).abs().max().detach()):.5f}  (<= half a step {step/2:.5f})")

    torch.set_printoptions(precision=4, sci_mode=False)
    print("\nw  (float weights):\n", w.detach())
    print("\nwq (fake-quant: snapped to the int8 grid, still float):\n", wq.detach())

    wq.sum().backward() #let loss=wq.sum()
    print("STE grad all ones:", bool(torch.allclose(w.grad, torch.ones_like(w))))
    #each element contributes to the sum, so the gradient should be 1 (the contribution is the constant in the derivative)

    q, s = quantize_int8(w.detach())
    print(f"int8 range {int(q.min())}..{int(q.max())}   scale {s:.6f}")
    print("\nq  (int8 integers, wq = q * scale):\n", q)
    assert int(q.abs().max()) == 127, "dynamic scale should push the largest weight to 127"
    print("ok")
