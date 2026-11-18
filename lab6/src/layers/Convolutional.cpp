#include "Convolutional.h"

#include <algorithm>
#include <iostream>
#ifndef ZEDBOARD
#include <thread>
#include <vector>
#endif

#include "../Types.h"
#include "../Utils.h"
#include "Layer.h"

namespace ML {

size ConvolutionalLayer::s_tileBlockSize = 16;
size ConvolutionalLayer::s_numThreads = 8;

// 1. Baseline Naive Convolution: loop order (p, q, m, r, s, c)
// Note: Inner loop steps through c while accessing weight[(r*S + s)*C*M + c*M + m], jumping by stride M!
void ConvolutionalLayer::computeNaive(const LayerData& dataIn) const {
    const dimVec& inDims = getInputParams().dims;
    const dimVec& outDims = getOutputParams().dims;
    const dimVec& weightDims = getWeightParams().dims;

    const size W = inDims[1], C = inDims[2];
    const size P = outDims[0], Q = outDims[1], M = outDims[2];
    const size R = weightDims[0], S = weightDims[1];

    LayerData& dataOut = getOutputData();

    for (size p = 0; p < P; p++) {
        for (size q = 0; q < Q; q++) {
            for (size m = 0; m < M; m++) {
                fp32 sum = biasData.get<fp32>(m);

                for (size r = 0; r < R; r++) {
                    for (size s = 0; s < S; s++) {
                        for (size c = 0; c < C; c++) {
                            sum += dataIn.get<fp32>(((p + r) * W + (q + s)) * C + c) *
                                   weightData.get<fp32>(((r * S + s) * C + c) * M + m);
                        }
                    }
                }

                dataOut.get<fp32>((p * Q + q) * M + m) = sum > 0.0f ? sum : 0.0f;
            }
        }
    }
}

// 2. Cache-Coherent Tiled Convolution (Lab 6 Section 3.1)
// Reorders inner loops so output channel 'm' is innermost (unit-stride sequential access on weights & outputs)
// and blocks (c, m, q) by tile size B to keep working sets inside L1/L2 cache.
void ConvolutionalLayer::computeTiled(const LayerData& dataIn) const {
    const dimVec& inDims = getInputParams().dims;
    const dimVec& outDims = getOutputParams().dims;
    const dimVec& weightDims = getWeightParams().dims;

    const size W = inDims[1], C = inDims[2];
    const size P = outDims[0], Q = outDims[1], M = outDims[2];
    const size R = weightDims[0], S = weightDims[1];
    const size B = s_tileBlockSize;

    const fp32* __restrict inPtr = static_cast<const fp32*>(dataIn.raw());
    const fp32* __restrict wPtr  = static_cast<const fp32*>(weightData.raw());
    const fp32* __restrict bPtr  = static_cast<const fp32*>(biasData.raw());
    fp32* __restrict outPtr      = static_cast<fp32*>(getOutputData().raw());

    // Initialize output feature map with biases
    for (size p = 0; p < P; ++p) {
        for (size q = 0; q < Q; ++q) {
            fp32* outPixel = outPtr + (p * Q + q) * M;
            for (size m = 0; m < M; ++m) {
                outPixel[m] = bPtr[m];
            }
        }
    }

    if (B == 0) {
        // Pure Memory-Reordered (p, q, r, s, c, m) without blocking
        for (size p = 0; p < P; ++p) {
            for (size q = 0; q < Q; ++q) {
                fp32* __restrict outPixel = outPtr + (p * Q + q) * M;
                for (size r = 0; r < R; ++r) {
                    for (size s = 0; s < S; ++s) {
                        const fp32* __restrict inPixel = inPtr + ((p + r) * W + (q + s)) * C;
                        const fp32* __restrict wBase   = wPtr + (r * S + s) * C * M;
                        for (size c = 0; c < C; ++c) {
                            const fp32 inVal = inPixel[c];
                            const fp32* __restrict wRow = wBase + c * M;
                            for (size m = 0; m < M; ++m) {
                                outPixel[m] += inVal * wRow[m];
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Tiled execution with block size B across Q, C, and M
        for (size p = 0; p < P; ++p) {
            for (size q0 = 0; q0 < Q; q0 += B) {
                const size qEnd = std::min(Q, q0 + B);
                for (size c0 = 0; c0 < C; c0 += B) {
                    const size cEnd = std::min(C, c0 + B);
                    for (size m0 = 0; m0 < M; m0 += B) {
                        const size mEnd = std::min(M, m0 + B);

                        for (size r = 0; r < R; ++r) {
                            for (size s = 0; s < S; ++s) {
                                for (size q = q0; q < qEnd; ++q) {
                                    fp32* __restrict outPixel = outPtr + (p * Q + q) * M;
                                    const fp32* __restrict inPixel = inPtr + ((p + r) * W + (q + s)) * C;
                                    const fp32* __restrict wBase   = wPtr + (r * S + s) * C * M;

                                    for (size c = c0; c < cEnd; ++c) {
                                        const fp32 inVal = inPixel[c];
                                        const fp32* __restrict wRow = wBase + c * M;
                                        for (size m = m0; m < mEnd; ++m) {
                                            outPixel[m] += inVal * wRow[m];
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Apply ReLU activation
    const size totalOut = P * Q * M;
    for (size i = 0; i < totalOut; ++i) {
        if (outPtr[i] < 0.0f) outPtr[i] = 0.0f;
    }
}

// 3. SIMD-friendly Convolution (Lab 6 Section 3.3)
// Contiguous unit-stride inner loop over m=0..M-1 enables compiler auto-vectorization (SSE2/AVX/NEON)
void ConvolutionalLayer::computeSIMD(const LayerData& dataIn) const {
    const dimVec& inDims = getInputParams().dims;
    const dimVec& outDims = getOutputParams().dims;
    const dimVec& weightDims = getWeightParams().dims;

    const size W = inDims[1], C = inDims[2];
    const size P = outDims[0], Q = outDims[1], M = outDims[2];
    const size R = weightDims[0], S = weightDims[1];

    const fp32* __restrict inPtr = static_cast<const fp32*>(dataIn.raw());
    const fp32* __restrict wPtr  = static_cast<const fp32*>(weightData.raw());
    const fp32* __restrict bPtr  = static_cast<const fp32*>(biasData.raw());
    fp32* __restrict outPtr      = static_cast<fp32*>(getOutputData().raw());

    for (size p = 0; p < P; ++p) {
        for (size q = 0; q < Q; ++q) {
            fp32* __restrict outPixel = outPtr + (p * Q + q) * M;
            for (size m = 0; m < M; ++m) {
                outPixel[m] = bPtr[m];
            }

            for (size r = 0; r < R; ++r) {
                for (size s = 0; s < S; ++s) {
                    const fp32* __restrict inPixel = inPtr + ((p + r) * W + (q + s)) * C;
                    const fp32* __restrict wBase   = wPtr + (r * S + s) * C * M;
                    for (size c = 0; c < C; ++c) {
                        const fp32 inVal = inPixel[c];
                        const fp32* __restrict wRow = wBase + c * M;
                        for (size m = 0; m < M; ++m) {
                            outPixel[m] += inVal * wRow[m];
                        }
                    }
                }
            }

            for (size m = 0; m < M; ++m) {
                if (outPixel[m] < 0.0f) outPixel[m] = 0.0f;
            }
        }
    }
}

// 4. Multi-threaded Convolution (Lab 6 Section 3.2 - Host PC only)
void ConvolutionalLayer::computeThreaded(const LayerData& dataIn) const {
#ifdef ZEDBOARD
    // ZedBoard is bare-metal without std::thread support
    computeTiled(dataIn);
#else
    const dimVec& inDims = getInputParams().dims;
    const dimVec& outDims = getOutputParams().dims;
    const dimVec& weightDims = getWeightParams().dims;

    const size W = inDims[1], C = inDims[2];
    const size P = outDims[0], Q = outDims[1], M = outDims[2];
    const size R = weightDims[0], S = weightDims[1];

    const fp32* __restrict inPtr = static_cast<const fp32*>(dataIn.raw());
    const fp32* __restrict wPtr  = static_cast<const fp32*>(weightData.raw());
    const fp32* __restrict bPtr  = static_cast<const fp32*>(biasData.raw());
    fp32* __restrict outPtr      = static_cast<fp32*>(getOutputData().raw());

    const size numThreads = std::max<size>(1, s_numThreads);
    const size totalPixels = P * Q;

    auto worker = [&](size tId) {
        const size startIdx = (totalPixels * tId) / numThreads;
        const size endIdx   = (totalPixels * (tId + 1)) / numThreads;

        for (size idx = startIdx; idx < endIdx; ++idx) {
            const size p = idx / Q;
            const size q = idx % Q;
            fp32* __restrict outPixel = outPtr + idx * M;

            for (size m = 0; m < M; ++m) {
                outPixel[m] = bPtr[m];
            }

            for (size r = 0; r < R; ++r) {
                for (size s = 0; s < S; ++s) {
                    const fp32* __restrict inPixel = inPtr + ((p + r) * W + (q + s)) * C;
                    const fp32* __restrict wBase   = wPtr + (r * S + s) * C * M;
                    for (size c = 0; c < C; ++c) {
                        const fp32 inVal = inPixel[c];
                        const fp32* __restrict wRow = wBase + c * M;
                        for (size m = 0; m < M; ++m) {
                            outPixel[m] += inVal * wRow[m];
                        }
                    }
                }
            }

            for (size m = 0; m < M; ++m) {
                if (outPixel[m] < 0.0f) outPixel[m] = 0.0f;
            }
        }
    };

    if (numThreads == 1) {
        worker(0);
        return;
    }

    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    for (size t = 0; t < numThreads; ++t) {
        threads.emplace_back(worker, t);
    }
    for (auto& th : threads) {
        th.join();
    }
#endif
}

}  // namespace ML
