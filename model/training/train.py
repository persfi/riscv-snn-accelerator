"""Train (or randomly generate) a RateSNN, log/plot curves, and export int8 weights.

Two modes:
  default   -- train on MNIST: per-epoch train + val eval, loss/accuracy plots, final test accuracy, then export the frozen int8 weights.
  --random  -- skip training; build a net of the given size with random weights and export it. 
"""
import argparse
from pathlib import Path

import matplotlib
matplotlib.use("Agg")  # headless: save PNGs, no window (WSL-friendly)
import matplotlib.pyplot as plt
import torch
import torch.nn.functional as F

from data import mnist_loaders
from model import RateSNN
from export import export_model


def accuracy(out_counts, labels):
    return (out_counts.argmax(dim=1) == labels).float().mean().item()


def train_epoch(model, loader, opt, device):
    model.train()
    loss_sum, acc_sum, n = 0.0, 0.0, 0
    for images, labels in loader:
        images, labels = images.to(device), labels.to(device)
        opt.zero_grad()
        out = model(images)
        loss = F.cross_entropy(out, labels)  #uses softmax cross-entropy 
        loss.backward()
        opt.step()
        bs = images.size(0)
        loss_sum += loss.item() * bs
        acc_sum += accuracy(out, labels) * bs
        n += bs #sample count
    return loss_sum / n, acc_sum / n #avg loss and accuracy per sample


@torch.no_grad() #evaluates without gradient tracking, no backward pass, used for either validation or testing
def evaluate(model, loader, device):
    model.eval()
    acc_sum, n = 0.0, 0
    for images, labels in loader:
        images, labels = images.to(device), labels.to(device)
        acc_sum += accuracy(model(images), labels) * images.size(0)
        n += images.size(0)
    return acc_sum / n


def plot_curves(hist, path):
    epochs = range(1, len(hist["train_loss"]) + 1)
    fig, (ax_loss, ax_acc) = plt.subplots(2, 1, figsize=(7, 8), sharex=True)
    ax_loss.plot(epochs, hist["train_loss"], marker=".", label="train loss")
    ax_loss.set_ylabel("loss"); ax_loss.legend(); ax_loss.grid(True, alpha=0.3)
    ax_acc.plot(epochs, hist["train_acc"], marker=".", label="train acc")
    ax_acc.plot(epochs, hist["val_acc"], marker=".", label="val acc")
    ax_acc.set_xlabel("epoch"); ax_acc.set_ylabel("accuracy")
    ax_acc.legend(); ax_acc.grid(True, alpha=0.3)
    fig.tight_layout(); fig.savefig(path, dpi=120); plt.close(fig)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--input", type=int, default=784)
    p.add_argument("--hidden", type=int, default=128)
    p.add_argument("--output", type=int, default=10)
    p.add_argument("--k", type=int, default=2 )
    p.add_argument("--T", type=int, default=20)
    p.add_argument("--v-th", type=float, default=1.0) #snn default
    p.add_argument("--epochs", type=int, default=15)
    p.add_argument("--batch-size", type=int, default=128)
    p.add_argument("--lr", type=float, default=1e-3)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--name", default=None, help="run dir under model/training/runs/")
    p.add_argument("--random", action="store_true",
                   help="skip training: random weights, export only (synthetic shape)")
    args = p.parse_args()

    torch.manual_seed(args.seed)                 # whole-run reproducibility
    device = "cuda" if torch.cuda.is_available() else "cpu"
    name = args.name or f"snn_h{args.hidden}_k{args.k}_T{args.T}"
    run_dir = Path("model/training/runs") / name
    run_dir.mkdir(parents=True, exist_ok=True) #make secondary run directory

    model = RateSNN(args.input, args.hidden, args.output,
                    k=args.k, T=args.T, v_th=args.v_th).to(device)

    if args.random:
        print(f"[random] {args.input}-{args.hidden}-{args.output}: exporting untrained net")
        export_model(model, run_dir, k=args.k, T=args.T, v_th=args.v_th)
        print(f"exported to {run_dir}")
        return

    train_loader, val_loader, test_loader = mnist_loaders(
        batch_size=args.batch_size, seed=args.seed)
    opt = torch.optim.Adam(model.parameters(), lr=args.lr)

    hist = {"train_loss": [], "train_acc": [], "val_acc": []}
    for epoch in range(1, args.epochs + 1):
        tr_loss, tr_acc = train_epoch(model, train_loader, opt, device)
        val_acc = evaluate(model, val_loader, device)
        hist["train_loss"].append(tr_loss)
        hist["train_acc"].append(tr_acc)
        hist["val_acc"].append(val_acc)
        print(f"epoch {epoch:2d}  loss {tr_loss:.4f}  train_acc {tr_acc:.4f}  val_acc {val_acc:.4f}")

    plot_curves(hist, run_dir / "curves.png")
    test_acc = evaluate(model, test_loader, device)
    print(f"\nTEST accuracy: {test_acc:.4f}")

    torch.save(model.state_dict(), run_dir / "model.pt")
    export_model(model, run_dir, k=args.k, T=args.T, v_th=args.v_th) #export.py
    print(f"saved checkpoint + int8 export to {run_dir}")


if __name__ == "__main__":
    main()
