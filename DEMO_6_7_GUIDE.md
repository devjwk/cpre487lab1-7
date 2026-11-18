# CPRE 487/587 — Labs 6 & 7 Final Demo Slides & Real Measured Data (Team 06)

모든 실험실 리눅스 PC 실측 데이터(`lab6/lab6_benchmark_results.csv`)와 Vivado 시뮬레이션/합성 결과(`lab7/`)가 반영된 **최종 발표 슬라이드 5장 패키지**입니다.
공유 슬라이드 덱(Shared Slide Deck)에 아래 5장의 텍스트와 생성된 이미지 파일들을 그대로 복사해 넣으시면 됩니다!

---

## 🖼️ 슬라이드에 첨부할 완성된 이미지 파일 목록 (Repository 내 경로)

1. **Slide 1 (Lab 6)**:
   - `lab6/lab6_memory_access_patterns.png` (Conv2의 Weight / Input / Output 메모리 접근 패턴 비교 차트 3종)
   - `lab6/lab6_tiling_latency.png` (Naive `144.7ms` vs Reordered `27.0ms` vs Tile 2~32 Latency 그래프)
   - `lab6/lab6_threading_latency.png` (Threads 1~64 Latency 그래프: 4 Threads에서 `8.21ms` / `17.62x` 달성!)
2. **Slide 3 (Lab 7)**:
   - `lab7/lab7_testbench_waveform.png` (5개 컨볼루션 시나리오 전체에서 `TEST_*_fail = 0`, `BRAM_OUTPUT_fail = 0` 통과 파형)
   - `lab7/lab7_synthesis_utilization.jpg` (Vivado Post-Synthesis Utilization: **LUT 9%, LUTRAM 2%, FF 4%, BRAM 49%, DSP 2%**)

---

## 📊 [Slide 1] Team 06 — Lab 6: Cache Reordering, Tiling, SIMD & Multithreading

* **Memory Access Reordering (`144.70 ms` → `26.98 ms`, `5.36×` Speedup)**:
  * Baseline loop `(p, q, m, r, s, c)` accessed weights `[(r*S+s)*C*M + c*M + m]` with stride `+M` (`+32`/`+64`/`+128` floats), causing severe L1 cache misses.
  * Moving output channel `m` to the innermost loop `(p, q, r, s, c, m)` achieved **unit-stride (`+1`) sequential access** on weights & outputs while reusing each input activation in a CPU register across `M` MAC operations.
* **Cache Tiling (`Block Sizes: 2, 4, 8, 16, 32`)**:
  * **Small tiles (`Tile 2 = 235.51 ms`, `0.61×`)** degraded performance below naive due to 9-nested loop branch overhead and short inner trip counts.
  * Performance improved monotonically with larger tiles (`Tile 4: 83.19 ms` → `Tile 8: 52.57 ms` → `Tile 16: 32.64 ms` → **`Tile 32: 27.39 ms`, `5.28×`**).
* **SIMD (`25.06 ms`, `5.77×`) & Multithreading (`1 to 64 Threads`)**:
  * Contiguous inner loop over `m` enabled compiler auto-vectorization (`vmulps`/`vaddps`).
  * **Peak performance at 4 Threads: `8.21 ms/inference` (`17.62×` speedup, `121.8 images/s`)**, matching the 4 physical CPU cores of the lab Linux machine. Beyond 4 threads (`8..64 threads`: `9.74 ms → 15.45 ms`), thread creation/synchronization and cache contention slightly increased latency.

| Optimization Mode | Parameter | Inference Latency (ms) | Speedup vs. Naive | Throughput (img/s) |
| :--- | :---: | :---: | :---: | :---: |
| **Baseline** | Naive `(p,q,m,r,s,c)` | 144.70 ms | 1.00× | 6.91 |
| **Loop Reordered** | Unit-Stride `(p,q,r,s,c,m)` | 26.98 ms | **5.36×** | 37.06 |
| **Cache Tiling** | Tile 2 / 4 / 8 / 16 / **32** | 235.5 / 83.2 / 52.6 / 32.6 / **27.39 ms** | 0.61× → **5.28×** | 36.52 |
| **SIMD Vectorized** | `-O3` Auto-Vectorized | 25.06 ms | **5.77×** | 39.90 |
| **Multithreaded** | 1 / 2 / **4** / 8 / 16 / 32 / 64 Threads | 26.1 / 14.2 / **8.21** / 9.74 / 11.8 / 12.1 / 15.5 ms | **17.62× (at 4T)** | **121.78** |

---

## 🛠️ [Slide 2] Lab 5 → Lab 7 Design Context & Accelerator Architecture

* **Lab 5 MAC Design-Space Exploration (Zynq-7020 Post-Implementation)**:
  | MAC Architecture | Precision | WNS (100 MHz clk) | Max Frequency ($F_{max}$) | Slice LUTs | Slice FFs | On-Chip Power |
  | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
  | **Staged MAC** | 8-bit / 4-bit / 2-bit | +2.05 ~ +2.36 ns | **125.75 ~ 130.91 MHz** | 2,231 ~ 2,436 | 2,879 ~ 3,323 | 1.702 W |
  | **Pipelined MAC** | 8-bit / 4-bit / 2-bit | +2.20 ~ +2.42 ns | **128.21 ~ 131.89 MHz** | 2,251 ~ 2,490 | 2,949 ~ 3,391 | 1.699 W |
* **Why 4× Pipelined 8-bit MACs for Lab 7**:
  * Pipelined MAC achieves **1 cycle/MAC throughput** at **~130 MHz** with virtually identical LUT/FF area and power to Staged MAC, while 8-bit quantization preserves full classification accuracy.
* **Theoretical Roofline & Bottleneck Analysis (Conv2: `60×60×32` → `56×56×32`, Filter `5×5×32×32`)**:
  * **Operations**: $56 \times 56 \times 32 \times (5 \times 5 \times 32) = \mathbf{80,281,600\text{ MACs}}$ (~61% of entire model's 131M MACs).
  * **Data Reuse & Stationary Flow**: Output-stationary / shared-input broadcast across 4 filter banks (fetching 1 input byte + 4 weight bytes from on-chip BRAM per cycle).
  * **Compute-Bound on PE Count**: With activation double-buffering (swapping Input/Output BRAM banks without DDR copies) and background CDMA weight prefetching, DMA overhead is <2%; theoretical full-model compute latency is **~262 ms at 125 MHz** (~655 ms at 50 MHz).

---

## ✅ [Slide 3] Team 06 — Lab 7: Hardware Implementation & Verification Status

* **All 4 Core Accelerator Submodules Implemented & Verified in Vivado**:
  1. **`index_gen.vhd` (PASS ✅)**: Multiplier-free 5-nested loop (`oh, ow, fc, fh, fw`) BRAM address generator using precomputed signed step offsets (`input_end_diff_fw/fh/fc/ow`).
  2. **`conv_mac.vhd` ×4 PEs (PASS ✅)**: 2-stage pipelined int8 MAC with direct 32-bit `bias` preload on the first accumulation cycle after `TLAST`/reset.
  3. **`dequantization.vhd` (PASS ✅)**: 2-stage pipelined fixed-point rescaling (`(acc * q_scale) >> 32`), optional ReLU, zero-point offset (`+ q_zero`), and clamping to `[-128, 127]`.
  4. **`output_storage.vhd` (PASS ✅)**: 3-state Read-Modify-Write FSM (`ST_IDLE → ST_READ_WAIT → ST_WRITE`) performing 8-bit byte-lane insertion into 32-bit word-addressed BRAM with on-the-fly **2×2 Max Pooling**.
* **Vivado Full System Post-Synthesis Resource Utilization (`conv_accelerator_bd_wrapper`)**:
  * **LUT: 9%** | **LUTRAM: 2%** | **FF: 4%** | **BRAM: 49%** (68.5 / 140 blocks) | **DSP: 2%** | **BUFG: 3%**
  * Leaves **51% of BRAM** and **98% of DSP slices** available for Final Project scaling!
* *(첨부 이미지: `lab7/lab7_testbench_waveform.png` & `lab7/lab7_synthesis_utilization.jpg`)*

---

## 💡 [Slide 4] Greatest Challenges & Most Important Aspect Learned

* **Greatest Challenges & How We Overcame Them**:
  * **Lab 6 (Cache & Threading Scaling)**:
    * Small tile sizes (`B = 2, 4`) were slower than naive due to loop nesting overhead, and naive loop ordering blocked compiler SIMD vectorization. Moving output channel `m` to the innermost loop unlocked both `5.36×` cache speedup and `17.62×` 4-thread speedup.
  * **Lab 7 (BRAM Read-Modify-Write & Handshake Timing in `output_storage.vhd`)**:
    * Handling the synchronous read latency of the 32-bit Output BRAM while comparing incoming `int8` values against existing pooled values for 2×2 Max Pooling (`oh % 2 == 1` or `ow % 2 == 1`).
    * Ensuring `conv_complete` asserts strictly **one clock cycle after** the final `BRAM_we = "1111"` write edge so the controller does not reset the BRAM enable prematurely.
* **Most Important Aspect Learned**:
  * **Memory Hierarchy & Data Movement Dictate Performance**: In Lab 5, per-packet AXI-FIFO CPU polling made hardware slower than software. In Lab 6 (L1/L2 cache unit-stride reuse) and Lab 7 (on-chip BRAM bank swapping + CDMA), keeping activations stationary near the compute units proved to be the single most critical principle in ML hardware design.

---

## 🚀 [Slide 5] Final Project Idea: Multi-Precision SIMD-Packed Convolution Accelerator

* **Observation from Lab 7 Synthesis**:
  * Our baseline 4-MAC accelerator uses **49% of BRAM**, but only **9% of LUTs** and **2% of DSPs** on the Zynq-7020!
  * Furthermore, each 32-bit BRAM read currently delivers only **one 8-bit operand per cycle** (75% of BRAM port bandwidth is discarded by `bram_slice_fetcher`).
* **Project Proposal: Sub-Byte Spatial Packing & 4× Input-Channel Parallelism**:
  1. **Full 32-bit Word Fetching (4× MAC Throughput per BRAM Bank)**:
     * Fetch **four 8-bit input channels (`ic..ic+3`) and four 8-bit weights simultaneously** in a single 32-bit BRAM read cycle, expanding each of the 4 output-channel units into a **4-way parallel dot-product tree (16 effective MACs/cycle → 4× speedup to <65 ms)** with **0% additional BRAM usage**!
  2. **Mixed-Precision (8-bit / 4-bit) Layer Execution**:
     * Leverage our Lab 4/5 quantization framework to pack **eight 4-bit weights per 32-bit BRAM word** on parameter-heavy middle layers (`Conv4`–`Conv6`), doubling effective on-chip weight storage capacity and arithmetic throughput.
