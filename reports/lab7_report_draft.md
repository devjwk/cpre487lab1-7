# CprE 487/587 – Lab 7 Report Draft (Team 06 – Jongwoo Kim)

---

## 3. Accelerator Design

### 3.1 High-level Architecture

#### Selected Bottleneck Layer: Conv2 (`conv2d_1`)
- Input dimensions: $60 \times 60 \times 32$ (`115,200` int8 elements = `115.2 KB`)
- Filter dimensions: $5 \times 5 \times 32 \times 32$ (`25,600` int8 weights = `25.6 KB`, plus 32 biases)
- Output dimensions: $56 \times 56 \times 32$ (`100,352` int8 elements = `100.35 KB`)

#### 1. How many operations are required to calculate that layer? How much data is required (per operation & total)?
- **Total MACs**: $56 \times 56 \times 32 \times (5 \times 5 \times 32) = 100{,}352 \times 800 = \mathbf{80{,}281{,}600\text{ MACs}}$ ($\mathbf{160.56\text{M OPs}}$ counting multiply and accumulate separately).
- **Data per MAC operation (without reuse)**: 1 byte input activation + 1 byte weight + 4 bytes partial sum read + 4 bytes partial sum write = **10 bytes/MAC** (or **2 bytes/MAC** if partial sum stays in a register).
- **Total Unique Layer Data**: $115.2\text{ KB (input)} + 25.6\text{ KB (weights)} + 100.35\text{ KB (output)} = \mathbf{241.15\text{ KB}}$.

#### 2. CPU Assumptions & Compute-Bound vs. Memory-Bound Analysis
- **Platform**: ZedBoard Zynq-7020 ARM Cortex-A9 @ `667 MHz`, 32 KB L1D cache, 512 KB L2 cache, DDR3 bandwidth $\approx 1.6\text{ GB/s}$ ($\approx 2.4\text{ bytes/cycle}$).
- **Naive CPU**: Requires $\approx 2\text{–}4$ cycles per MAC in software plus frequent cache misses (10–50 cycles on L2/DDR). Without reuse, $2\text{ bytes/MAC}$ over a $2.4\text{ bytes/cycle}$ DDR bus (or worse with 32-bit floats: $8\text{ bytes/MAC}$) yields an arithmetic intensity below the ridge point, making naive CPU execution **memory-bound**. With Lab 6 loop reordering and int8 quantization, working sets fit in L1/L2 cache and execution shifts toward **compute-bound**.

#### 3. Data Reuse, Dataflow, Tile Size, and Number of PEs
- **Data Reuse**: Each input activation pixel is broadcast simultaneously to **4 parallel MAC units (`4×` input reuse)**, and each output partial sum is held stationary in a 32-bit accumulator register across all $C \times R \times S = 800$ accumulations (**`800×` partial-sum reuse**, eliminating intermediate partial-sum BRAM traffic).
- **Dataflow**: **Output-Stationary (OS)** — each PE (`conv_mac`) initializes its 32-bit accumulator with the channel bias and accumulates the full filter volume locally before emitting a single 32-bit result to `dequantization`.
- **Tile Size & Number of PEs**: The ZedBoard Zynq-7020 provides `140` BRAM18K/36K tiles (`4.9 Mb` total). Double-buffering input/output activations (2 banks × 128 KB) plus 4 double-buffered filter banks fits comfortably within **49% BRAM**. With 4 filter BRAM banks feeding **4 parallel 8-bit Piped MAC PEs**, the accelerator computes **4 output channels in parallel** per pass (tiling the 32 output channels of Conv2 into $32 / 4 = 8$ passes).

---

### 3.2 Visualizing Your Design

- **Full System Diagram**: *(Insert `lab7/lab7_template/diagram.svg` showing Zynq PS, HP0 AXI CDMA, Double-Buffered Activation BRAM0/BRAM1, 4× Double-Buffered Filter BRAMs, AXI-Lite Config, and MLP Accelerator)*
- **Micro-Architectural Diagram**: *(Include 4 implemented submodules: `index_gen` → `mac_stream_provider` → 4× `conv_mac` → `output_combiner` → `dequantization` → `output_storage`)*

#### Control Signals & Configuration Registers Table (`conv_config.vhd` / `MLP.h`)

| Register / Signal | Width | Source | Function / Effect on Accelerator |
| :--- | :---: | :--- | :--- |
| `conv_idle` (`MLP_CTRLA`) | 1 | AXI-Lite / PS | `1` = idle/reset; `0` = starts `index_gen` and convolution execution |
| `conv_complete` (`MLP_CTRLA`) | 1 | `output_storage` | Asserted 1 cycle after final BRAM write completes; polled by CPU |
| `MLP_CTRLB_SWAP_ACTIVATIONS` | 1 | AXI-Lite / PS | Swaps Activation BRAM0/BRAM1 roles (Input ↔ Output) between layers |
| `MLP_CTRLB_SWAP_FILTERS` | 1 | AXI-Lite / PS | Swaps double-buffered Filter banks for ping-pong DMA loading |
| `MLP_CTRLB_RELU` | 1 | AXI-Lite / PS | Enables ReLU clamping (`max(0, x)`) inside `dequantization` |
| `MLP_CTRLB_POOL` | 1 | AXI-Lite / PS | Enables on-the-fly 2×2 MaxPooling inside `output_storage` |
| `filter_w`, `filter_h`, `filter_c` | 16 each | AXI-Lite / PS | Filter kernel dimensions ($W_f, H_f, C$) for 5-loop bounds in `index_gen` |
| `output_w`, `output_h` | 16 each | AXI-Lite / PS | Output feature map spatial dimensions ($W_o, H_o$) |
| `input_end_diff_{fw,fh,fc,ow}` | 32 signed | AXI-Lite / PS | Signed jump offsets for multiplier-free 5-loop BRAM addressing in `index_gen` |
| `mac0_bias` .. `mac3_bias` | 32 signed | AXI-Lite / PS | Preloaded into each PE accumulator on the first cycle of every filter volume |
| `q_scale`, `q_zero` | 32 / 8 | AXI-Lite / PS | Fixed-point requantization multiplier (`>> 32`) and next-layer zero point |
| `output_channel_offset` | 16 | AXI-Lite / PS | Base output channel index for current 4-channel tile pass |

*Table 1. Accelerator Control Signals and AXI-Lite Configuration Registers*

---

### 3.3 Theoretical Calculations

- **Data Required per Accelerator Cycle**: 1 byte input activation + 4 bytes weights (one per MAC) = **5 bytes/cycle** read from on-chip BRAMs.
- **Cycles per Operation**: The 2-stage pipelined `conv_mac` units sustain **II = 1** (4 MACs / 8 OPs per clock cycle across the 4 PEs).
- **Compute-Bound vs. Memory-Bound**: Because the 5 dedicated on-chip BRAM interfaces supply all 5 bytes every single cycle with zero stalls during convolution, the accelerator core is **compute-bound** (limited by the 4 PEs and clock frequency), with a small per-layer **AXI CDMA weight-transfer overhead** (<2% of total runtime).
- **Bottleneck Reduction**: Throughput can be increased by (1) raising the PL clock from 50 MHz to 100–125 MHz, (2) expanding from 4 PEs to 8 or 16 PEs with wider/banked BRAMs, or (3) compressing sparse zero-heavy activations/weights to reduce BRAM usage and skip zero-MAC cycles.
- **Theoretical Latency & Throughput on Conv2 ($80.28\text{M MACs}$)**:
  - Cycles $= 80{,}281{,}600 / 4 = 20{,}070{,}400\text{ cycles}$.
  - At `100 MHz`: **$200.7\text{ ms}$** (`4.98 Conv2/sec`) — **6.2× faster** than Lab 2 ZedBoard FP32 software (`1,252.97 ms`).
  - At `50 MHz` (default template clock): **$401.4\text{ ms}$** (`3.1×` faster than Lab 2 SW).

---

## 4. Implementation, Verification & Benchmarking

### 4.3 Hardware Implementation & Vivado Verification (100% Complete)

I implemented all four required VHDL modules in `lab7/lab7_template/hdl/`:
1. **`index_gen.vhd`**: Multiplier-free 5-nested loop (`oh, ow, fc, fh, fw`) BRAM address generator using signed jump offsets (`input_end_diff_*`).
2. **`conv_mac.vhd`**: 2-stage pipelined int8×int8 MAC with 32-bit `bias` preload on the first valid cycle after reset or `TLAST`.
3. **`dequantization.vhd`**: 2-stage pipelined `(acc * q_scale) >> 32`, optional ReLU, `+ q_zero`, and saturation to `[-128, 127]`.
4. **`output_storage.vhd`**: 3-state Read-Modify-Write FSM (`ST_IDLE`, `ST_READ_WAIT`, `ST_WRITE`) into 32-bit Output BRAM with on-the-fly 2×2 MaxPooling and a 1-cycle delayed `conv_complete` assertion so the final write latches cleanly.

- **Testbench Verification**: Passed **100% of all 5 autogenerated test cases** in `conv_accelerator_tb.vhd` with zero failures (`TEST_*_fail = 0`, `BRAM_OUTPUT_fail = 0`) — *(Insert `lab7/lab7_testbench_waveform_cropped.png`)*.
- **Vivado Full-System Synthesis (`conv_accelerator_bd_wrapper`)**: Completed with **9% LUT, 2% LUTRAM, 4% FF, 49% BRAM, 2% DSP, 3% BUFG** — *(Insert `lab7/lab7_synthesis_utilization_zoomed.jpg`)*.

---

### 4.5 Benchmark Your Design (`[TODO: Fill ZedBoard HW Measurements]`)

| Implementation | Platform | Latency (ms/img) | Throughput (img/s) | Top-1 Accuracy (%) | Speedup vs. Lab 2 ZedBoard |
| :--- | :--- | :---: | :---: | :---: | :---: |
| Lab 1 TensorFlow | Lab x86 PC | `[TODO]` | `[TODO]` | ~24% | — |
| Lab 2 FP32 C++ | ZedBoard ARM | 2,175.87 | 0.46 | `[TODO]` | 1.00× |
| Lab 4 INT8 C++ | ZedBoard ARM | `[TODO]` | `[TODO]` | `[TODO]` | `[TODO]` |
| Lab 5 8-bit Piped MAC (FIFO) | ZedBoard FPGA | `[TODO]` | `[TODO]` | `[TODO]` | `[TODO]` |
| Lab 6 Optimized (4T + SIMD) | Lab x86 PC | 8.21 | 121.78 | `[TODO]` | — |
| **Lab 7 Custom Accelerator** | **ZedBoard FPGA** | **`[TODO]`** | **`[TODO]`** | **`[TODO]`** | **`[TODO]`** |

- **[TODO]** Generate Bitstream & export `.xsa`, integrate `MLP.h` CDMA driver in C++, run full inference on ZedBoard, fill Table 2 + comparison plot, and post fastest HW inference time to the class MS Teams channel.

---

### 4.6 Reflection and Final Design

- **Data Movement & Bottleneck**: Data enters and exits the accelerator BRAMs exclusively via **AXI CDMA (`memcpy_dma`)** with `Xil_DCacheDisable()` on the ARM CPU. Because activations stay on-chip via double-buffered BRAM swapping (`MLP_CTRLB_SWAP_ACTIVATIONS`), DDR transfers are eliminated between layers, making the core **compute-bound** (4 PEs) while **BRAM capacity (49%)** is the primary FPGA resource bottleneck.
- **Issues Faced**: (1) Getting the 5-loop signed address wrap-around offsets exact in `index_gen` without multipliers; (2) synchronizing the 3-state BRAM Read-Modify-Write cycle for 2×2 MaxPooling in `output_storage` and delaying `conv_complete` by 1 cycle so the final byte finishes writing before `conv_idle` asserts; (3) generating the block design HDL wrapper (`conv_accelerator_bd_wrapper`) as top module in Vivado synthesis.
- **What Was Easier Than Anticipated**: Once `index_gen` and `output_storage` passed the stage-by-stage Vivado testbench, the pipelined `conv_mac` and `dequantization` units integrated cleanly on the first simulation run with zero mismatches.
- **Design Choices I Would Change**: Adding a lightweight lossless compression/decompression stage (RLE or Bitmask) with sparse zero-skipping between the BRAMs and MACs to reduce the 49% BRAM footprint and skip zero-value ReLU MAC cycles (which is now the focus of my Term Project).
- **What Was Accomplished**: Designed, implemented, 100% testbench-verified, and synthesized a complete 4-PE Output-Stationary int8 CNN hardware accelerator with on-the-fly dequantization and 2×2 MaxPooling on the Zynq-7020 FPGA.
