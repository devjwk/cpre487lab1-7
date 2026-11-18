#pragma once

#include "../Types.h"
#include "../Utils.h"
#include "Layer.h"

namespace ML {
class ConvolutionalLayer : public Layer {
   public:
    ConvolutionalLayer(const LayerParams inParams, const LayerParams outParams, const LayerParams weightParams, const LayerParams biasParams)
        : Layer(inParams, outParams, LayerType::CONVOLUTIONAL),
          weightParam(weightParams),
          weightData(weightParams),
          biasParam(biasParams),
          biasData(biasParams) {}

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

    // Virtual functions
    virtual void computeNaive(const LayerData& dataIn) const override;
    virtual void computeThreaded(const LayerData& dataIn) const override;
    virtual void computeTiled(const LayerData& dataIn) const override;
    virtual void computeSIMD(const LayerData& dataIn) const override;

    // Runtime configuration for Lab 6 sweeps (Tiling block size & Thread count)
    static void setTileBlockSize(size blockSize) { s_tileBlockSize = blockSize; }
    static size getTileBlockSize() { return s_tileBlockSize; }
    static void setNumThreads(size threads) { s_numThreads = threads; }
    static size getNumThreads() { return s_numThreads; }

   private:
    LayerParams weightParam;
    LayerData weightData;

    LayerParams biasParam;
    LayerData biasData;

    static size s_tileBlockSize;
    static size s_numThreads;
};

}  // namespace ML
