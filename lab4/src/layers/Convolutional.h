#pragma once

#include "../Types.h"
#include "../Utils.h"
#include "Layer.h"

namespace ML {
class ConvolutionalLayer : public Layer {
   public:
    // Constructor used by the existing FP32 model.
    ConvolutionalLayer(
        const LayerParams inParams,
        const LayerParams outParams,
        const LayerParams weightParams,
        const LayerParams biasParams
    )
        : Layer(inParams, outParams, LayerType::CONVOLUTIONAL),
          weightParam(weightParams),
          weightData(weightParams),
          biasParam(biasParams),
          biasData(biasParams) {}

    // Constructor used by the INT8 model.
    ConvolutionalLayer(
        const LayerParams inParams,
        const LayerParams outParams,
        const LayerParams weightParams,
        const LayerParams biasParams,
        fp32 inputScale,
        fp32 weightScale,
        i32 inputZeroPoint,
        fp32 outputScale,
        i32 outputZeroPoint,
        int bits = 8
    )
        : ConvolutionalLayer(inParams, outParams, weightParams, biasParams) {
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

    // Fre all resources allocated for the layer
    virtual void freeLayer() override {
        Layer::freeLayer();
        weightData.freeData();
        biasData.freeData();
    }

    // Virtual functions
    virtual void computeNaive(const LayerData& dataIn) const override;
    virtual void computeThreaded(const LayerData& dataIn) const override;
    virtual void computeTiled(const LayerData& dataIn) const override;
    virtual void computeSIMD(const LayerData& dataIn) const override;

   private:
    LayerParams weightParam;
    LayerData weightData;

    LayerParams biasParam;
    LayerData biasData;
    // Quantization settings.
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