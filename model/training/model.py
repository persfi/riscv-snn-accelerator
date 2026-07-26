
import torch
import torch.nn as nn
import snntorch as snn

from encode import rate_encode
from quant import QuantLinear


def beta_from_k(k):
    # leak `V -= V>>k`  <=>  `V *= (1 - 2^-k)`  =>  beta = 1 - 2^-k
    return 1.0 - 2.0 ** (-k)


class RateSNN(nn.Module):
    def __init__(self, input_size=784, hidden_size=128, output_size=10,
                 k=2, T=20, v_th=1.0):
        super().__init__()
        self.T = T
        beta = beta_from_k(k)

        lif_cfg = dict(beta=beta, threshold=v_th,
                       reset_mechanism="subtract", reset_delay=False)
        self.fc1 = QuantLinear(input_size, hidden_size) 
        #layer that adds all weights*spike, contains the ste for backpropagation
        self.lif1 = snn.Leaky(**lif_cfg) #leak + check spike
        self.fc2 = QuantLinear(hidden_size, output_size)
        self.lif2 = snn.Leaky(**lif_cfg)

    def forward(self, images):
        # images: [batch, input] in [0,1] -> spikes [T, batch, input] in {0,1}.
        spikes = rate_encode(images, self.T)

        mem1 = self.lif1.reset_mem()   # needs to reset neuron state per sample (linear layer is stateless weights that are used every timestep)
        mem2 = self.lif2.reset_mem()
        out_count = torch.zeros(images.shape[0], self.fc2.out_features,
                                device=images.device) #create the out_count on the same device as image, initialize to 0

        for t in range(self.T):
            cur1 = self.fc1(spikes[t])          #weighted input spikes 
            spk1, mem1 = self.lif1(cur1, mem1)  #hidden LIF: leak, integrate fire, reset
            cur2 = self.fc2(spk1)               #hidden spikes -> output current
            spk2, mem2 = self.lif2(cur2, mem2)  #output LIF
            out_count = out_count + spk2        #accumulate output spikes over T

        return out_count                        #[batch, output] spike counts == logits


if __name__ == "__main__":
    #check that gradients reach the fc1 weights through the STE, and that the output shape is correct
    torch.manual_seed(0)
    net = RateSNN(input_size=784, hidden_size=128, output_size=10, k=2, T=20)
    x = torch.rand(8, 784)                       # fake pixels in [0,1]
    out = net(x)
    print("output shape:", tuple(out.shape), " (batch size, output)")
    assert out.shape == (8, 10)

    out.sum().backward()
    g = net.fc1.weight.grad
    print("fc1.weight grad reaches float weights:", g is not None and bool((g != 0).any()))
    assert g is not None and (g != 0).any(), "STE must let gradients reach the fc1 weights"
    print("output spike counts (row 0):", out[0].tolist())
    print("ok")
