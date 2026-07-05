

## 1. Decisions & Rejected Alternatives

### D1: Use the Arty A7 100T FPGA board
**Chose:** Arty A7 100T 
**Rejected:** Basys3, PYNQ Z2, DE1-SoC, DE2-115
**Because:** 
||Arty A7 100T|Basys3 |DE2-115| DE1-SoC| PYNQ Z2|
| ------|-------- | -------  | ------- | ------- |---|
|LC/LE|101k|33k|114k|85k|85k|
|BRAM|4.8Mb|1.8Mb|3.8Mb|3.9Mb|4.9Mb|
|Tool|Vivado|Vivado|Quartus Prime|Quartus Prime|Vivado|
|Reason|Most balanced, accessible (in Taiwan), and within budget.| Same series as A7 100T but three times smaller. BRAM too small for the project.|Massive physical I/Os that aren't needed in this project. Harder to source than the Arty A7 100T.|Built in system on chip (SoC), used in my ECE241 course next term, displacing the RISC-V core which is a point of the project. |Built in SoC and Jupyter/Python environment. However, this project will not be running Python on the chip and will only execute C programs (Pytorch is traintime only).

<small>LC/LE units depends on the vendor and are not directly comparable; written for order-of-magnitude comparison only</small>

**Revisit if:** 
Reconsider PYNQ Z2 if accelerators, embedded vision, or AI inference became the project core. 
Consider DE1-SoC for September port target (accelerator fabric, own core stays as host), to demonstrate vendor portability. 


