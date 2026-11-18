#pragma once

#include "../Types.h"
#include "../Utils.h"
#include "Layer.h"
#include <cmath>
namespace ML {
// Fully connected layer: every output is a weighted sum of all inputs plus a bias
class DenseLayer : public Layer {
   public:
    // applyRelu is false for the last dense layer, whose output goes to the softmax layer instead
    DenseLayer(const LayerParams inParams, const LayerParams outParams, const LayerParams weightParams, const LayerParams biasParams,
               const bool applyRelu = true)
        : Layer(inParams, outParams, LayerType::DENSE),
          weightParam(weightParams),
          weightData(weightParams),
          biasParam(biasParams),
          biasData(biasParams),
          applyRelu(applyRelu) {}
            DenseLayer(const LayerParams inParams, const LayerParams outParams,
               const LayerParams weightParams, const LayerParams biasParams,
               bool applyRelu, fp32 inputScale, fp32 weightScale,
               i32 inputZeroPoint, fp32 outputScale = 1.0f,
               i32 outputZeroPoint = 0,
               int  bits = 8)
        : DenseLayer(inParams, outParams, weightParams, biasParams, applyRelu) {
        quantized = true;
        si = inputScale;
        sw = weightScale;
        zi = inputZeroPoint;
        so = outputScale;
        zo = outputZeroPoint;
        quantMin = -(1 << (bits - 1));
        quantMax = (1 << (bits - 1)) - 1;
    }

    // Getters
    const LayerParams& getWeightParams() const { return weightParam; }
    const LayerParams& getBiasParams() const { return biasParam; }
    const LayerData& getWeightData() const { return weightData; }
    const LayerData& getBiasData() const { return biasData; }

    // Allocate all resources needed for the layer & Load all of the required data for the layer
    virtual void allocLayer() override {
        Layer::allocLayer();
        weightData.loadData();
        biasData.loadData();
    }

    // Free all resources allocated for the layer
    virtual void freeLayer() override {
        Layer::freeLayer();
        weightData.freeData();
        biasData.freeData();
    }

    virtual void computeNaive(const LayerData& dataIn) const override {
        // Input (N), weights (N, M), output (M)
        const size N = getInputParams().dims[0];
        const size M = getOutputParams().dims[0];

        LayerData& dataOut = getOutputData();
                if (quantized) {
            const fp32 accumulatorScale = si * sw;

            for (size m = 0; m < M; ++m) {
                i32 sum = biasData.get<i32>(m);

                for (size n = 0; n < N; ++n) {
                    const i32 input = dataIn.get<i8>(n);
                    const i32 weight = weightData.get<i8>(n * M + m);
                    sum += (input - zi) * weight;
                }

                const fp32 restored =
                    static_cast<fp32>(sum) / accumulatorScale;

                if (applyRelu) {
                    const fp32 relu = restored > 0.0f ? restored : 0.0f;
                    fp32 clipped = std::round(relu * so) + zo;

                    if (clipped < quantMin) clipped = quantMin;
                    if (clipped > quantMax) clipped = quantMax;

                    dataOut.get<i8>(m) = static_cast<i8>(clipped);
                } else {
                    // Softmax expects FP32 logits.
                    dataOut.get<fp32>(m) = restored;
                }
            }

            return;
        }

        for (size m = 0; m < M; m++) {
            fp32 sum = biasData.get<fp32>(m);

            for (size n = 0; n < N; n++) {
                sum += dataIn.get<fp32>(n) * weightData.get<fp32>(n * M + m);
            }

            dataOut.get<fp32>(m) = (applyRelu && sum < 0) ? 0 : sum;
        }
    }

    // Only the naive implementation is required for this lab
    virtual void computeThreaded(const LayerData& dataIn) const override { computeNaive(dataIn); }
    virtual void computeTiled(const LayerData& dataIn) const override { computeNaive(dataIn); }
    virtual void computeSIMD(const LayerData& dataIn) const override { computeNaive(dataIn); }

   private:
    LayerParams weightParam;
    LayerData weightData;

    LayerParams biasParam;
    LayerData biasData;

    bool applyRelu;
    bool quantized = false;
    fp32 si = 1.0f;
    fp32 sw = 1.0f;
    i32 zi = 0;
    fp32 so = 1.0f;
    i32 zo = 0;
    i32 quantMin = -128;
    i32 quantMax = 127;
};

}  // namespace ML
