# CprE 487/587 – Lab 4 Report Draft (Team 06 – Jongwoo Kim)

---

## 3.1 Estimate the memory size needed to store the TinyImageNet parameters

I checked the model and found **769,376 weights** and **840 biases**, totaling **770,216 parameters**. Assuming parameters are packed at the selected bit width and $1\text{ KB} = 1{,}000\text{ bytes}$:

- **FP32**: $770{,}216 \times (32 / 8) = 3{,}080{,}864\text{ bytes}$ ($3{,}080.864\text{ KB}$)
- **INT8**: $770{,}216 \times (8 / 8) = 770{,}216\text{ bytes}$ ($770.216\text{ KB}$)
- **INT2**: $770{,}216 \times (2 / 8) = 192{,}554\text{ bytes}$ ($192.554\text{ KB}$)

### What is the max quantization level among 1, 2, 4, 8, 16 bits that allows the TinyImageNet model to fit on a device with 1,000 KB of overall memory?

| Precision | Bits per Parameter | Memory (bytes) | Memory (KB) | Fits within 1,000 KB? |
| :--- | :---: | :---: | :---: | :---: |
| FP32 | 32 | 3,080,864 | 3,080.864 | No |
| INT16 | 16 | 1,540,432 | 1,540.432 | No |
| **INT8** | **8** | **770,216** | **770.216** | **Yes** |
| INT4 | 4 | 385,108 | 385.108 | Yes |
| INT2 | 2 | 192,554 | 192.554 | Yes |
| INT1 | 1 | 96,277 | 96.277 | Yes |

*Table 1. Model Parameter Memory at Different Precisions*

As shown in Table 1, **INT8** is the highest precision that fits within 1,000 KB for parameter storage, requiring 770.216 KB.

### Are there any other types of memory than the raw model memory that need to be taken into account to enable inference?

Yes. Inference also requires memory for the **input image** and **intermediate activations (outputs)**, because each layer's output must be buffered and fed as the next layer's input.

### What impact could quantization have on this memory size (please quantify for your model)?

If intermediate activations are also quantized, their memory footprint drops proportionally. For my first Conv2D layer output ($60 \times 60 \times 32 = 115{,}200$ values):
- **FP32**: $460.8\text{ KB}$
- **INT8**: $115.2\text{ KB}$ (**75% reduction**)
- **INT2**: $28.8\text{ KB}$ (**93.75% reduction**)

---

## 3.2 Plot Histogram & Weight/Bias Min-Max Analysis

*(Insert Figure 1: FP32 Weight Distributions by Layer — `lab4/util/weight_histograms.png`)*

### What do you observe from the distribution? Are the weight values uniformly distributed?

Most weight values are concentrated near zero, with fewer values toward either tail of the range. Therefore, **the weights are not uniformly distributed** (they follow an approximately zero-centered bell-shaped distribution).

### Show & note the min-max values of both weights and biases

| Layer | Weight Min | Weight Max | Bias Min | Bias Max |
| :--- | :---: | :---: | :---: | :---: |
| `conv2d` | -0.302879 | 0.234254 | -0.057268 | 0.097612 |
| `conv2d_1` | -0.486778 | 0.382551 | -0.070180 | 0.095580 |
| `conv2d_2` | -0.692378 | 0.324826 | -0.209900 | 0.152339 |
| `conv2d_3` | -0.541547 | 0.362909 | -0.295808 | 0.102229 |
| `conv2d_4` | -0.536679 | 0.362402 | -0.317999 | 0.180665 |
| `conv2d_5` | -0.510655 | 0.468192 | -0.136140 | 0.212835 |
| `dense` | -0.557585 | 0.512802 | -0.179310 | 0.182238 |
| `dense_1` | -1.324119 | 0.366575 | -0.239749 | 0.163579 |

*Table 2. Minimum and Maximum Weight and Bias Values by Layer*

---

## 3.3 Activation Min-Max Across 1,000 Random Training Images

I randomly selected 1,000 training images, normalized their pixel values to $[0, 1]$, ran them through the FP32 model, and recorded the overall min-max activation values per layer:

| Layer | Activation Min | Activation Max |
| :--- | :---: | :---: |
| `conv2d` | 0.000000 | 1.709803 |
| `conv2d_1` | 0.000000 | 3.952060 |
| `max_pooling2d` | 0.000000 | 3.952060 |
| `conv2d_2` | 0.000000 | 2.921465 |
| `conv2d_3` | 0.000000 | 4.107682 |
| `max_pooling2d_1` | 0.000000 | 4.107682 |
| `conv2d_4` | 0.000000 | 5.769876 |
| `conv2d_5` | 0.000000 | 8.912045 |
| `max_pooling2d_2` | 0.000000 | 8.912045 |
| `flatten` | 0.000000 | 8.912045 |
| `dense` | 0.000000 | 21.451197 |
| `dense_1` | $5.01 \times 10^{-18}$ | 0.999929 |

*Table 3. Activation Min-Max Values Across 1,000 Training Images*

---

## 3.4 – 3.5 Quantized Inference Accuracy & Latency (`[TODO: Fill Measured Validation Results]`)

| Precision | Top-1 Accuracy (%) | Top-5 Accuracy (%) | Top-10 Accuracy (%) | Latency on x86 (ms) | Latency on ZedBoard (ms) |
| :--- | :---: | :---: | :---: | :---: | :---: |
| FP32 (Baseline) | `[TODO]` | `[TODO]` | `[TODO]` | `[TODO]` | `2175.87` (Lab 2) |
| INT8 | `[TODO]` | `[TODO]` | `[TODO]` | `[TODO]` | `[TODO]` |
| INT4 | `[TODO]` | `[TODO]` | `[TODO]` | `[TODO]` | `[TODO]` |
| INT2 | `[TODO]` | `[TODO]` | `[TODO]` | `[TODO]` | `[TODO]` |

- **[TODO]** Run the quantized C++ framework (`lab4/`) on the 1,000 validation images (`lab4/data/validation/`) and fill in the Top-1/Top-5/Top-10 accuracy and latency numbers above.
