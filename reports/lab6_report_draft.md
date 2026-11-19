# CprE 487/587 – Lab 6 Report Draft (Team 06 – Jongwoo Kim)

---

## 3.1 Tiling & Memory Access Optimization

### 3.1.1 Identify Cache Sizes of the ZedBoard (Zynq-7020 Dual-Core ARM Cortex-A9)

| Cache Level | Size | Notes |
| :--- | :---: | :--- |
| **L1 Data Cache (L1D)** | **32 KB** (per core) | 4-way set-associative, 32-byte cache line |
| **L1 Instruction Cache (L1I)** | **32 KB** (per core) | 4-way set-associative, 32-byte cache line |
| **L2 Cache** | **512 KB** (shared) | 8-way set-associative, unified, shared across both Cortex-A9 cores |
| **L3 Cache** | **None (0 KB)** | ZedBoard Zynq-7020 does not have an L3 cache (misses go to 512 MB DDR3) |

*Table 1. ZedBoard (Zynq-7020 ARM Cortex-A9) Cache Hierarchy*

---

### 3.1.2 Identify & Reorder Memory Accesses (Speedup + 3 Access Pattern Plots)

From Lab 2 profiling, **Conv2 (`conv2d_1`, $60\times60\times32 \rightarrow 56\times56\times32$, $5\times5$ kernel, $80.28\text{M MACs}$)** dominates total inference runtime.

- **Naive Access Pattern (`p, q, m, r, s, c`)**:
  - Weights are stored in `(R, S, C, M)` row-major order, so incrementing the innermost input channel `c` jumps by **stride $+M$ (`+32` elements)** on every iteration. This causes frequent L1 cache misses and wastes spatial locality.
- **Cache-Optimized Loop Reordering (`p, q, r, s, c, m`)**:
  - Moving output channel `m` to the innermost loop converts weight reads to **sequential unit-stride (`+1`)** accesses, keeps each input activation element loaded in a CPU register for **32× temporal reuse** across all 32 output channels, and accumulates the 32 output channels sequentially in L1 cache.
- **Measured Speedup (ISU Lab x86 PC, `lab6/lab6_benchmark_results.csv`)**:
  - **Naive Baseline**: `144.701 ms/img` (`6.91 img/s`, `1.00×`)
  - **Reordered (`m` innermost)**: `26.981 ms/img` (`37.06 img/s`, **`5.36×` speedup**)
  - *(Insert Figure 1: `lab6/lab6_memory_access_patterns.png` — Weight, Input, and Output Access Patterns for Conv2)*
  - **[TODO]** Add 100-inference averaged profiling numbers on the ZedBoard for before vs. after loop reordering.

**Why the 3 Plots Demonstrate Optimal Cache Coherency**:
1. **Weight Plot**: Slope drops from $+32$ per step (red) to $+1$ per step (green), ensuring every 64-byte cache line fetch is 100% utilized before eviction.
2. **Input Plot**: Flat staircase (blue) shows each input scalar is fetched once and reused 32 consecutive times from a register.
3. **Output Plot**: Tight sawtooth over `[0..31]` (orange) keeps the active 32-element output vector hot in L1 cache with unit-stride writes.

---

### 3.1.3 Cache Tiling (Block Sizes 2, 4, 8, 16, 32)

| Implementation / Block Size | x86 Latency (ms/img) | x86 Speedup vs. Naive | x86 Throughput (img/s) | ZedBoard Latency (ms) |
| :--- | :---: | :---: | :---: | :---: |
| Naive Baseline | 144.701 | 1.00× | 6.91 | `[TODO]` |
| Reordered (No Tiling) | 26.981 | 5.36× | 37.06 | `[TODO]` |
| Tile 2 | 235.512 | 0.61× | 4.25 | `[TODO]` |
| Tile 4 | 83.190 | 1.74× | 12.02 | `[TODO]` |
| Tile 8 | 52.573 | 2.75× | 19.02 | `[TODO]` |
| Tile 16 | 32.644 | 4.43× | 30.63 | `[TODO]` |
| **Tile 32 (Optimal)** | **27.385** | **5.28×** | **36.52** | `[TODO]` |

*Table 2. Single-Image Inference Latency vs. Tiling Block Size (`lab6/lab6_tiling_latency.png`)*

- **Optimal Block Size & Explanation**: **Tile 32** was optimal among all tested block sizes (`27.385 ms`, `5.28×` speedup over Naive). Very small tiles (`Tile 2: 235.5 ms`, `Tile 4: 83.2 ms`) were slower than the naive baseline because splitting into 9 nested loops introduces heavy loop-control, index arithmetic, and `std::min` boundary-check overhead that far outweighs cache reuse gains. At `Tile 32`, the tile matches the full output channel width (`M = 32`), minimizing loop overhead while keeping the active working set inside L1 cache.

---

## 3.2 Multi-threading (Lab x86 Machine Only)

### 3.2.1 & 3.2.2 Thread Scaling (`1, 2, 4, 8, 16, 32, 64` Threads) + Optimal Thread Count

I parallelized the outer output-row dimension (`p`) of convolutional layers using C++ `std::thread`, assigning disjoint output rows to each thread to avoid lock contention or false sharing.

| Threads (`std::thread`) | Latency (ms/img) | Speedup vs. Naive | Throughput (img/s) |
| :---: | :---: | :---: | :---: |
| 1 | 26.097 | 5.54× | 38.32 |
| 2 | 14.234 | 10.17× | 70.26 |
| **4 (Optimal)** | **8.211** | **17.62×** | **121.78** |
| 8 | 9.737 | 14.86× | 102.70 |
| 16 | 11.831 | 12.23× | 84.52 |
| 32 | 12.128 | 11.93× | 82.45 |
| 64 | 15.455 | 9.36× | 64.71 |

*Table 3. Inference Latency vs. Number of Threads (`lab6/lab6_threading_latency.png`)*

- **Optimal Thread Count & Analysis**: **4 threads** achieved the lowest latency of **`8.211 ms/img` (`17.62×` speedup, `121.78 img/s`)**. Beyond 4 threads (`8` to `64` threads), latency degraded from `9.74 ms` to `15.45 ms`. Because threads are spawned and joined on every layer and each layer runs in only a few milliseconds, OS thread creation/synchronization overhead, cache contention, and diminishing per-thread work outweigh additional parallelism past 4 threads.

---

## 3.3 SIMD Optimization

| Compiler / SIMD Configuration | x86 Latency (ms/img) | Speedup vs. Naive | Notes |
| :--- | :---: | :---: | :--- |
| No SIMD (`-O3`, Reordered) | 26.981 | 5.36× | Scalar unit-stride baseline |
| SIMD Auto-Vectorized (`-O3`, `computeSIMD`) | **25.062** | **5.77×** | Vectorized `m` innermost loop |
| `-msse2` | `[TODO]` | `[TODO]` | Run with explicit `-msse2` & `ASM=true` |
| `-mavx` | `[TODO]` | `[TODO]` | Run with explicit `-mavx` & `ASM=true` |
| `-march=native` | `[TODO]` | `[TODO]` | Run with explicit `-march=native` & `ASM=true` |
| ZedBoard ARM NEON | `[TODO]` | `[TODO]` | Test `-mfpu=neon` in Vitis on ZedBoard |

- **[TODO]** Generate assembly snippets (`make ASM=true`) showing scalar (`mulss`/`addss`) vs. vector (`vmulps`/`vaddps` or `vfmadd231ps`) instructions for each SIMD flag and include them here.

---

## 3.4 Need for Speed (Best Combined Result vs. Lab 1 TensorFlow)

- **Best Combined x86 Performance (Unit-Stride Reordering + SIMD + 4 Threads)**: **`8.211 ms/img`** (**`121.78 img/s`**, **`17.62×` speedup** over Naive C++).
- **Comparison with Lab 1 TensorFlow**: TensorFlow on the lab machine uses highly tuned BLAS/oneDNN kernels (`im2col` + AVX2/FMA GEMM + persistent thread pools), typically achieving ~2–3 ms/img on Conv2/full inference. Our C++ implementation closes most of the gap from `144.7 ms` down to `8.21 ms` through cache locality and 4-thread parallelism; remaining overhead comes from per-layer thread spawn/join (no thread pool) and absence of explicit AVX2 FMA intrinsics.
- **[TODO]** Run a 100-image batch average on the lab machine to report official competition latency & throughput.
