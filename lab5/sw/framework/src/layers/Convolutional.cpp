#include "Convolutional.h"

#include <iostream>

#include "../Types.h"
#include "../Utils.h"
#include "Layer.h"
#include <cmath>
#include <vector>
#include "../Mac.h"

namespace ML {
// --- Begin Student Code ---

// Compute the convultion for the layer data
void ConvolutionalLayer::computeNaive(const LayerData& dataIn) const {
    // Dimensions, named as in the textbook: input (H, W, C), weights (R, S, C, M), output (P, Q, M)
    const dimVec& inDims = getInputParams().dims;
    const dimVec& outDims = getOutputParams().dims;
    const dimVec& weightDims = getWeightParams().dims;

    const size W = inDims[1], C = inDims[2];
    const size P = outDims[0], Q = outDims[1], M = outDims[2];
    const size R = weightDims[0], S = weightDims[1];

    LayerData& dataOut = getOutputData();

    if (quantized) {
    for (size p = 0; p < P; ++p) {
        for (size q = 0; q < Q; ++q) {
            for (size m = 0; m < M; ++m) {
                i32 sum = biasData.get<i32>(m);

                for (size r = 0; r < R; ++r) {
                    for (size s = 0; s < S; ++s) {
                        for (size c = 0; c < C; ++c) {
                            const size inputIndex =
                                ((p + r) * W + (q + s)) * C + c;
                            const size weightIndex =
                                ((r * S + s) * C + c) * M + m;

                            const i32 input = dataIn.get<i8>(inputIndex);
                            const i32 weight = weightData.get<i8>(weightIndex);
                            sum += (input - zi) * weight;
                        }
                    }
                }
                // Dequantize, apply ReLU, and quantize for the next layer.
                const fp32 restored = static_cast<fp32>(sum) / (si * sw);
                const fp32 relu = restored > 0.0f ? restored : 0.0f;
                const fp32 rounded = std::round(relu * so) + zo;

                fp32 clipped = rounded;
                if (clipped < quantMin) clipped = quantMin;
                if (clipped > quantMax) clipped = quantMax;

                const size outputIndex = (p * Q + q) * M + m;
                dataOut.get<i8>(outputIndex) = static_cast<i8>(clipped);
            }
        }
    }
    return;
}

    // Stride 1, no padding: output (p, q) covers input rows p..p+R-1 and columns q..q+S-1
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

                // ReLU activation is part of the layer, as in the Keras Conv2D layers of the model
                dataOut.get<fp32>((p * Q + q) * M + m) = sum > 0 ? sum : 0;
            }
        }
    }
}

// Compute the convolution using threads
void ConvolutionalLayer::computeThreaded(const LayerData& dataIn) const {
    // Only the naive implementation is required for this lab
    computeNaive(dataIn);
}

// Compute the convolution using a tiled approach
void ConvolutionalLayer::computeTiled(const LayerData& dataIn) const {
    // Only the naive implementation is required for this lab
    computeNaive(dataIn);
}

// Compute the convolution using SIMD
void ConvolutionalLayer::computeSIMD(const LayerData& dataIn) const {
    // Only the naive implementation is required for this lab
    computeNaive(dataIn);
}

void ConvolutionalLayer::computeAccelerated(const LayerData& dataIn) const {   
    if (!quantized) { computeNaive(dataIn); return; }

    const dimVec& inDims = getInputParams().dims;
    const dimVec& outDims = getOutputParams().dims;
    const dimVec& wDims = getWeightParams().dims;
    const size W = inDims[1], C = inDims[2];
    const size P = outDims[0], Q = outDims[1], M = outDims[2];
    const size R = wDims[0], S = wDims[1];
    const size G = R * S * C;                       
    LayerData& dataOut = getOutputData();
    const fp32 accScale = si * sw;

    std::vector<i8> kernels(M * G);
    std::vector<i32> offsets(M);
    for (size m = 0; m < M; ++m) {
        i32 wsum = 0;
        for (size k = 0; k < G; ++k) {
            const i8 w = weightData.get<i8>(k * M + m);
            kernels[m * G + k] = w;
            wsum += w;
        }
        offsets[m] = biasData.get<i32>(m) - zi * wsum;
    }

    std::vector<i8> patch(G);
    for (size p = 0; p < P; ++p) {
        for (size q = 0; q < Q; ++q) {
            for (size r = 0; r < R; ++r)
                for (size s = 0; s < S; ++s)
                    for (size c = 0; c < C; ++c)
                        patch[(r * S + s) * C + c] =
                            dataIn.get<i8>(((p + r) * W + (q + s)) * C + c);

            for (size m = 0; m < M; ++m) {
                const i32 sum = macGroup(&kernels[m * G], patch.data(), G) + offsets[m];
                const fp32 restored = static_cast<fp32>(sum) / accScale;
                const fp32 relu = restored > 0.0f ? restored : 0.0f;
                fp32 clipped = std::round(relu * so) + zo;
                if (clipped < quantMin) clipped = quantMin;
                if (clipped > quantMax) clipped = quantMax;
                dataOut.get<i8>((p * Q + q) * M + m) = static_cast<i8>(clipped);
            }
        }
    }
}                                                                              

}  // namespace ML
