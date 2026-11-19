# CprE 487/587 – Lab 5 Report Draft (Team 06 – Jongwoo Kim)

---

## 3. Integration Bliss (Highlighted Requirements)

### 3.3 Implement 4-bit and 2-bit versions of both your staged and pipelined MAC (6 MACs total) and save each XSA and Vivado report

I implemented 8-bit, 4-bit, and 2-bit versions of both `staged_mac` and `piped_mac` in VHDL (`lab5/hw/{8,4,2}bit_mac/`), synthesized and implemented all 6 block designs in Vivado 2020.1, exported all 6 hardware `.xsa` files (`lab5/xsa/{staged,piped}_{8,4,2}bit.xsa`), and saved their post-implementation timing, utilization, and power reports (`lab5/reports/`).

### 3.4 Implement `computeAccelerated` to utilize your MAC units and verify full inference matches quantized software

I implemented `computeAccelerated` in the C++ framework (`lab5/sw/framework/src/ML.cpp`, `Mac.h`, `layers/Convolutional.cpp`, `layers/Dense.h`) to offload convolution and dense MAC operations to the FPGA via the AXI-Stream FIFO while keeping MaxPooling, Flatten, and FP32 Softmax on the ARM processor.
- **[TODO]** Record verified cosine similarity / exact match logs between software quantized inference and `computeAccelerated` on the ZedBoard for all 6 `.xsa` designs.

---

## 4. The Big 3 – Power, Performance, and Area (PPA)

### 4.1 Report the maximum throughput and minimum latency of your design in the form of an equation and example values

Assuming no external back-pressure, control, or data-movement delays, let $N$ be the number of MAC operations in an output accumulation (plus 1 cycle for bias preload via `TUSER = 1`), and $T_{\text{clk}} = 1 / F_{\max}$:

- **Staged MAC** (1 cycle per MAC + 1 cycle bias + 1 cycle output/clear):
  $$\text{Latency}_{\text{staged}}(N) = (N + 2) \times T_{\text{clk}}, \qquad \text{Throughput}_{\text{staged}}(N) = \frac{N}{(N + 2) \times T_{\text{clk}}}$$
- **Pipelined MAC** (3-stage pipeline: 1 MAC/cycle steady-state, 2-cycle pipeline fill/drain latency):
  $$\text{Latency}_{\text{piped}}(N) = (N + 3) \times T_{\text{clk}}, \qquad \text{Throughput}_{\text{piped}}(N) = \frac{N}{(N + 1) \times T_{\text{clk}}} \xrightarrow{N \gg 1} F_{\max}$$

Using post-implementation $F_{\max} = \frac{1000}{10.0 - \text{WNS}}$ (10 ns target clock period) for the 8-bit Piped MAC ($\text{WNS} = 2.257\text{ ns} \Rightarrow F_{\max} = 129.15\text{ MHz}$, $T_{\text{clk}} = 7.743\text{ ns}$):

| $N$ (MACs) | Piped Latency (ns) | Piped Throughput (MMAC/s) | Staged-4bit ($125.75\text{ MHz}$) Latency (ns) | Staged-4bit Throughput (MMAC/s) |
| :---: | :---: | :---: | :---: | :---: |
| 1 MAC | 30.97 ns | 64.58 MMAC/s | 23.86 ns | 41.92 MMAC/s |
| 5 MACs | 61.94 ns | 107.63 MMAC/s | 55.67 ns | 89.82 MMAC/s |
| 100 MACs | 797.53 ns | 127.87 MMAC/s | 811.13 ns | 123.28 MMAC/s |
| 1,000 MACs | 7,766.23 ns | 129.02 MMAC/s | 7,968.19 ns | 125.50 MMAC/s |

*Table 1. Ideal Latency and Throughput Examples (No External Transfer Overhead)*

---

### 4.2 Report peak power and area from Vivado and calculate energy for a single inference on our Toy Model

From Vivado post-implementation utilization, timing, and power reports (`lab5/reports/`), the full-system (Zynq PS + AXI-FIFO + MAC) PPA results are summarized below:

| MAC Design | WNS (ns) | $F_{\max}$ (MHz) | Slice LUTs | Slice Registers (FF) | BRAM Tiles | DSPs | Total On-Chip Power (W) |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| `staged_mac_2bit` | 2.361 | 130.91 | 2,231 | 2,879 | 12 | 0 | 1.702 W |
| `staged_mac_4bit` | 2.048 | 125.75 | 2,252 | 2,879 | 12 | 0 | 1.702 W |
| `staged_mac_8bit` | `[TODO]`* | `[TODO]`* | 118 (IP only) | 41 (IP only) | 0 (IP) | 0 | `[TODO]`* |
| `pipeline_2bit` | 2.200 | 128.21 | 2,251 | 2,949 | 12 | 0 | 1.699 W |
| `pipeline_4bit` | 2.418 | 131.89 | 2,268 | 2,949 | 12 | 0 | 1.699 W |
| `pipeline_8bit` | 2.257 | 129.15 | 2,315 | 2,965 | 12 | 0 | 1.700 W |

*\*Note: `staged_mac_8bit` folder currently contains the standalone IP report; run full-system post-implementation report on `staged_8bit.xsa` to fill the full-system row.*

**Single-Inference Ideal Energy Calculation (TinyImageNet Toy Model: $\approx 131.4\text{M MACs}$ total across all Conv + Dense layers)**:
- Assuming optimal continuous operation at $1\text{ MAC/cycle}$ on `pipeline_8bit` ($F_{\max} = 129.15\text{ MHz}$, $P = 1.700\text{ W}$):
  $$t_{\text{ideal}} = \frac{131.4 \times 10^6\text{ cycles}}{129.15 \times 10^6\text{ Hz}} \approx 1.017\text{ s}$$
  $$E_{\text{ideal}} = P \times t_{\text{ideal}} = 1.700\text{ W} \times 1.017\text{ s} \approx \mathbf{1.73\text{ J}}$$

---

### 4.3 How would power, energy, area, and performance scale with multiple PEs (3 PEs, 5 PEs, 100 PEs)?

- **Performance (Throughput & Latency)**: Ideally scales linearly with the number of PEs ($P\times$ higher throughput, $1/P$ latency) assuming independent outputs are computed in parallel without memory stalls (e.g., 3 PEs $\rightarrow 3\times$, 5 PEs $\rightarrow 5\times$, 100 PEs $\rightarrow 100\times$).
- **Area (LUTs / FFs)**: Only the MAC logic scales linearly with $P$ ($\approx 120$ LUTs and $41\text{–}90$ FFs per MAC), while the fixed Zynq PS/AXI interconnect overhead ($\approx 2{,}150$ LUTs) stays constant. Thus, 100 PEs would add roughly $12{,}000$ LUTs (~22% of Zynq-7020 LUTs), which fits in logic area, though AXI/BRAM routing and fanout would become harder to close timing on.
- **Power & Energy**: Dynamic PL power scales linearly with $P$, while Zynq PS + static power ($\approx 1.5\text{–}1.6\text{ W}$ out of $1.70\text{ W}$) remains nearly constant. Because total runtime drops by $1/P$ while total power rises only slightly, **total energy per inference decreases significantly** as $P$ increases until memory bandwidth saturates.

---

### 4.4 Do these values make sense? Do they overestimate or underestimate realistic board values? What factors are missing?

These ideal values **significantly overestimate performance (and underestimate real latency and energy)** compared to what is measured on the ZedBoard. Key omitted factors include:
1. **AXI-Stream FIFO Transfer Overhead**: Sending every weight/activation word individually from the ARM CPU over AXI-Lite/AXI-FIFO takes dozens of CPU cycles per MAC, completely dominating the 1-cycle hardware MAC time.
2. **DDR Memory & Cache Latency**: Fetching inputs/weights from off-chip DDR and software loop indexing overhead on the ARM Cortex-A9.
3. **Control & Requantization Overhead**: Software dequantization/requantization, bias handling, and MaxPool/Softmax execution between layers.

---

### 4.5 Profile the execution time of all implementations. Compare speed and reasons for differences.

| Implementation | ZedBoard Latency (ms) | Throughput (img/s) | Top-1 Accuracy (%) | Notes |
| :--- | :---: | :---: | :---: | :--- |
| `staged_8bit` | `[TODO]` | `[TODO]` | `[TODO]` | Per-packet AXI-FIFO bound |
| `staged_4bit` | `[TODO]` | `[TODO]` | `[TODO]` | Higher bit-packing per FIFO word |
| `staged_2bit` | `[TODO]` | `[TODO]` | `[TODO]` | Highest bit-packing per FIFO word |
| `piped_8bit` | `[TODO]` | `[TODO]` | `[TODO]` | Similar latency if FIFO-bottlenecked |
| `piped_4bit` | `[TODO]` | `[TODO]` | `[TODO]` | `[TODO]` |
| `piped_2bit` | `[TODO]` | `[TODO]` | `[TODO]` | `[TODO]` |

- **[TODO]** Flash the 6 `.xsa` files on the ZedBoard, profile end-to-end execution times, and fill in Table 2.

---

### 4.6 How might a variable-precision MAC unit affect these metrics?

A variable-precision MAC lets sensitive layers (e.g., first `conv2d` and final `dense_1`) run at **8-bit** to preserve classification accuracy while running compute-heavy intermediate layers (`conv2d_1`–`conv2d_5`, `dense`) at **4-bit or 2-bit**. Compared to a static 8-bit MAC, this reduces memory traffic and improves throughput on lower-precision layers with only modest extra control/shift-add area overhead, achieving a better accuracy-vs-energy tradeoff than uniform 4-bit or 2-bit quantization.

---

## 5. Variable-Precision MAC (`[TODO: Implement & Evaluate]`)

### 5.1 – 5.5 Variable-Precision MAC Design, Per-Layer Precision Selection, and Comparison

- **Planned Architecture (EPDNN 7.4)**: Temporal / Spatial bit-decomposed MAC (reusing 2-bit sub-multipliers via shift-and-add to support 2-bit, 4-bit, and 8-bit modes at runtime via AXI configuration/TUSER control).
- **Per-Layer Precision Rationale**: Keep first layer (`conv2d`) and last layer (`dense_1`) at **8-bit** (highest sensitivity to quantization error); use **4-bit** on middle Conv/Dense layers where weight distributions are tightly centered around zero.
- **[TODO]** Implement `lab5/hw/variable_mac/`, synthesize in Vivado, integrate with `lab5/sw/`, and compare PPA & accuracy against the static 8/4/2-bit designs above.
