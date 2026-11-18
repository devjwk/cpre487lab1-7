# CPRE 487/587 — Labs 6 & 7 Demo Package & Execution Guide (Team 06)

이 문서는 **Labs 6 & 7 Demo (4~5 Slides, 5분 발표)**를 위해 맥북에서 사전 구현된 코드들과, **실험실 Linux PC / ZedBoard에서 돌려야 할 명령어**, 그리고 **발표 슬라이드 5장 구성안**을 정리한 가이드입니다.

---

## 1. 실험실 리눅스 PC에서 가장 먼저 할 일 (동기화)

실험실 리눅스 터미널에서 아래 명령어로 새로 추가된 `lab6/`와 `lab7/`을 받아옵니다:

```bash
cd ~/cpre487
git fetch origin
git reset --hard origin/main
ls -la
# -> lab2, lab3, lab4, lab5, lab6, lab7 폴더 확인!
```

---

## 2. [Lab 6] 실험실 Linux PC & ZedBoard 실행 방법

### A. 실험실 Linux PC (x86)에서 실행 (소요 시간: 약 1~2분)
```bash
cd ~/cpre487/lab6

# 1. C++ 컴파일 및 100회 반복 벤치마크 실행 (Naive, Reordered, Tiling 2~32, SIMD, Threads 1~64)
make clean && make
./build/ml

# 2. 발표 슬라이드용 그래프 3종(.png) 자동 생성!
python3 scripts/run_lab6_experiments.py
```
실행이 끝나면 `lab6/` 폴더 안에 **슬라이드에 바로 붙여넣을 수 있는 3개의 고해상도 그래프 이미지**가 생성됩니다:
1. `lab6_memory_access_patterns.png` (Conv2의 Weight / Input / Output 메모리 접근 패턴 비교 차트 3개)
2. `lab6_tiling_latency.png` (Naive vs Reordered vs Tile Size 2, 4, 8, 16, 32 Latency 막대그래프)
3. `lab6_threading_latency.png` (Threads 1, 2, 4, 8, 16, 32, 64 Latency 막대그래프)

### B. (선택) SIMD 컴파일 플래그별 비교 & 어셈블리 확인 (실험실 Linux PC)
```bash
cd ~/cpre487/lab6
# 어셈블리 파일(build/layers/Convolutional.s) 생성하여 addss/mulss vs vaddps/vmulps 확인
make clean && make ASM=true
grep -E "mulps|addps|fmadd|mulss|addss" build/layers/Convolutional.s | head -n 30
```

### C. (선택) ZedBoard에서 Tiling (Block Size 2, 4, 8, 16, 32) 측정
- `lab6/`에서 `./scripts/build_vitis` 후 `./scripts/flash_vitis`를 실행하면, 한 번의 실행으로 Naive, Reordered, Tile 2, 4, 8, 16, 32의 보드 실행 시간이 시리얼 터미널에 한꺼번에 출력됩니다!

---

## 3. [Lab 7] 실험실 Linux PC (Vivado) 실행 방법

`lab7/lab7_template/hdl/` 안에 4대 미구현 핵심 모듈이 모두 구현되어 있습니다:
1. `index_gen.vhd`: 곱셈기 없이 오프셋 레지스터 덧셈만으로 5중 루프(`oh, ow, fc, fh, fw`) BRAM 주소를 매 사이클 생성
2. `conv_mac.vhd`: 2-Stage Pipelined 8-bit MAC (첫 누적 시 AXI-Lite `bias` 합산)
3. `dequantization.vhd`: 2-Stage Pipelined 32-bit → 8-bit 역양자화/재양자화 (`(acc * q_scale) >> 32` → ReLU → `+ q_zero` → `[-128, 127]` Saturation)
4. `output_storage.vhd`: 3-State Read-Modify-Write (`ST_IDLE` → `ST_READ_WAIT` → `ST_WRITE`) BRAM 바이트 슬라이스 저장 및 하드웨어 2x2 Max Pooling

### 실험실 Linux PC에서 Vivado 시뮬레이션 & 합성 리포트 뽑기
```bash
cd ~/cpre487/lab7/lab7_template/vivado
/remote/Xilinx/2020.1/Vivado/2020.1/bin/vivado &
```
1. Vivado 하단 **Tcl Console**에서 실행:
   ```tcl
   source lab6_template.tcl
   ```
2. **Testbench 시뮬레이션 확인 (발표 슬라이드 캡처용 1)**:
   - 왼쪽 Flow Navigator에서 **Run Simulation → Run Behavioral Simulation** 클릭
   - `TEST_s_index_gen_m_axis_fail`, `TEST_s_mac0_m_axis_fail`, `TEST_s_dequantization_m_axis_fail`, `BRAM_OUTPUT_fail` 신호가 `'0'`으로 유지되며 `"Simulation Complete!"`가 뜨는 파형(Waveform) 화면 캡처!
3. **Synthesis / Implementation 리소스 & 타이밍 확인 (발표 슬라이드 캡처용 2)**:
   - Sources 탭에서 `block_design` 우클릭 → **Create HDL Wrapper** → 생성된 wrapper를 **Set as Top**으로 지정
   - **Run Synthesis** (및 **Run Implementation**) 클릭
   - 완료 후 **Project Summary** 화면의 **Utilization 표/막대그래프 (LUT, LUTRAM, FF, BRAM, DSP %)** 및 **Timing (WNS / WHS)** 캡처!

---

## 4. 바로 복사해서 쓰는 Demo 6 & 7 발표 슬라이드 (5장 구성안)

### [Slide 1] Team 06 — Lab 6: Cache Reordering, Tiling & Multithreading
* **Memory Layout & Loop Reordering**:
  * Baseline `(p, q, m, r, s, c)` accessed weights `[(r*S+s)*C*M + c*M + m]` with stride `+M` (`+32`/`+64`/`+128` floats), causing severe L1 cache thrashing.
  * Reordered inner loop to `(p, q, r, s, c, m)` → **Unit-stride (`+1`) sequential access** on weights & output channels, while holding `input[(p+r, q+s, c)]` in a register across `M` MACs.
* **Cache Tiling (Block Sizes 2, 4, 8, 16, 32)**:
  * Small tiles (`B = 2, 4`) incur loop-control overhead exceeding cache benefits; **`B = 16` / `32`** best matches ZedBoard's 32 KB L1 D-Cache & 512 KB L2 Cache.
* **Multithreading (`std::thread`, 1 to 64 Threads) & SIMD (`-O3 -march=native`)**:
  * Partitioned output spatial positions `(P × Q)` across worker threads with unit-stride vectorized inner loop (`vmulps` / `vaddps`).
* *(여기에 `lab6_memory_access_patterns.png`, `lab6_tiling_latency.png`, `lab6_threading_latency.png` 삽입)*

### [Slide 2] Lab 5 → Lab 7 Design Context & Accelerator Architecture
* **Lab 5 MAC Design-Space Exploration (Zynq-7020 Post-Implementation)**:
  | MAC Architecture | Precision | WNS (100 MHz clk) | Max Frequency ($F_{max}$) | Slice LUTs | Slice FFs | On-Chip Power |
  | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
  | **Staged MAC** | 8-bit / 4-bit / 2-bit | +2.05 ~ +2.36 ns | **125.7 ~ 130.9 MHz** | 2,231 ~ 2,436 | 2,879 ~ 3,323 | 1.702 W |
  | **Pipelined MAC** | 8-bit / 4-bit / 2-bit | +2.20 ~ +2.42 ns | **128.2 ~ 131.9 MHz** | 2,251 ~ 2,490 | 2,949 ~ 3,391 | 1.699 W |
* **Why 4× Pipelined 8-bit MACs for Lab 7**:
  * Pipelined MAC achieves **1 cycle/MAC throughput** at **~130 MHz** with negligible area/power penalty over Staged MAC, and 8-bit quantization preserves Top-1/Top-10 accuracy close to FP32.
* **Theoretical Roofline Analysis (Conv2: `60×60×32` → `56×56×32`, `5×5×32×32`)**:
  * Total MACs = $56 \times 56 \times 32 \times (5 \times 5 \times 32) = 80,281,600\text{ MACs}$.
  * With 4 parallel PEs fetching 1 input + 4 weights per cycle from on-chip BRAM: **Computation-bound on PE count** (~20.07M cycles for Conv2; ~32.8M cycles across all Conv+Dense layers $\approx 262\text{ ms}$ at 125 MHz), eliminating Lab 5's AXI-FIFO bottleneck.

### [Slide 3] Team 06 — Lab 7: Module Implementation & Operational Status
* **Implemented Hardware Modules (Template Architecture)**:
  1. **`index_gen.vhd`**: Multiplier-free 5-nested loop (`oh, ow, fc, fh, fw`) address generator using precomputed jump offsets (`input_end_diff_fw/fh/fc/ow`).
  2. **`conv_mac.vhd` (×4 PEs)**: 2-stage pipelined int8 MAC with direct 32-bit `bias` preload on the first cycle after `TLAST`/reset.
  3. **`dequantization.vhd`**: 2-stage pipelined fixed-point rescaling (`(acc * q_scale) >> 32`), ReLU, zero-point shift (`+ q_zero`), and int8 saturation `[-128, 127]`.
  4. **`output_storage.vhd`**: 3-state Read-Modify-Write FSM (`ST_IDLE → ST_READ_WAIT → ST_WRITE`) handling 8-bit sub-word lane insertion into 32-bit BRAM with on-the-fly **2×2 Max Pooling**.
* *(여기에 Vivado Testbench 시뮬레이션 통과 화면 및 Vivado Synthesis Utilization 표 캡처 삽입)*

### [Slide 4] Greatest Challenges & Key Takeaways
* **Greatest Challenges & How We Overcame Them**:
  * **Lab 6**: Understanding why small tile sizes (`2, 4`) degraded performance and why compiler auto-vectorization (SIMD) failed on the naive `(p,q,m,r,s,c)` loop until we moved output channel `m` to the innermost loop for contiguous memory access.
  * **Lab 7**:
    1. Synchronizing the 2-cycle synchronous read latency of the 32-bit Output BRAM during the Read-Modify-Write + 2×2 Max Pooling state machine in `output_storage.vhd`.
    2. Ensuring `conv_complete` asserts strictly **one clock cycle after** the final `BRAM_we = "1111"` write edge so the controller does not reset the BRAM enable prematurely.
* **Most Important Aspect Learned**:
  * **Data Movement Dominates Compute**: In Lab 5, single-PE AXI-Stream FIFO transfers made hardware slower than CPU; in Lab 6 & Lab 7, cache locality (unit-stride reordering) and on-chip BRAM double-buffering (CDMA + bank swapping) proved that memory hierarchy design is the true driver of ML acceleration.

### [Slide 5] Final Project Idea: Multi-Precision SIMD-Packed Convolution Accelerator
* **Motivation**:
  * Lab 7's baseline 32-bit BRAM ports fetch only **one 8-bit activation / weight per cycle** (utilizing just 25% of the 32-bit BRAM word bandwidth per cycle) and only **4 PEs** (~3% of ZedBoard's 220 DSP48E1 slices).
* **Proposed Project Architecture**:
  1. **Spatial Sub-Byte Packing (4-bit / 8-bit Variable Precision)**: Pack multiple quantized operands (e.g., four 8-bit or eight 4-bit weights/activations) per 32-bit BRAM read to multiply effective memory bandwidth by **4×–8×** without adding BRAM blocks.
  2. **Mixed-Precision Layer Scheduling**: Combine our Lab 5 variable-precision MAC insights with Lab 7's DMA engine—running early feature-extraction layers (`Conv1`, `Dense2`) in 8-bit for accuracy while running parameter-heavy middle layers (`Conv4`–`Conv6`) in 4-bit for 2× throughput and energy reduction.
