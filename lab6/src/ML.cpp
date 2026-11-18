#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "Config.h"
#include "Model.h"
#include "Types.h"
#include "Utils.h"
#include "layers/Convolutional.h"
#include "layers/Dense.h"
#include "layers/Flatten.h"
#include "layers/Layer.h"
#include "layers/MaxPooling.h"
#include "layers/Softmax.h"

#ifdef ZEDBOARD
#include <file_transfer/file_transfer.h>
#endif

namespace ML {

#ifdef ZEDBOARD
// On ZedBoard (667MHz ARM), run 10 iterations by default for fast turnaround (or set to 100 for final report)
const std::size_t BENCH_ITERATIONS = 10;
#else
// On Lab Linux PC (x86), run 100 iterations as requested in Lab 6 Section 3.1
const std::size_t BENCH_ITERATIONS = 100;
#endif

Model buildToyModel(const Path modelPath) {
    Model model;
    logInfo("--- Building Toy Model for Lab 6 ---");

    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {64, 64, 3}},
        LayerParams{sizeof(fp32), {60, 60, 32}},
        LayerParams{sizeof(fp32), {5, 5, 3, 32}, modelPath / "conv1_weights.bin"},
        LayerParams{sizeof(fp32), {32}, modelPath / "conv1_biases.bin"}
    );
    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {60, 60, 32}},
        LayerParams{sizeof(fp32), {56, 56, 32}},
        LayerParams{sizeof(fp32), {5, 5, 32, 32}, modelPath / "conv2_weights.bin"},
        LayerParams{sizeof(fp32), {32}, modelPath / "conv2_biases.bin"}
    );
    model.addLayer<MaxPoolingLayer>(
        LayerParams{sizeof(fp32), {56, 56, 32}},
        LayerParams{sizeof(fp32), {28, 28, 32}}
    );
    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {28, 28, 32}},
        LayerParams{sizeof(fp32), {26, 26, 64}},
        LayerParams{sizeof(fp32), {3, 3, 32, 64}, modelPath / "conv3_weights.bin"},
        LayerParams{sizeof(fp32), {64}, modelPath / "conv3_biases.bin"}
    );
    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {26, 26, 64}},
        LayerParams{sizeof(fp32), {24, 24, 64}},
        LayerParams{sizeof(fp32), {3, 3, 64, 64}, modelPath / "conv4_weights.bin"},
        LayerParams{sizeof(fp32), {64}, modelPath / "conv4_biases.bin"}
    );
    model.addLayer<MaxPoolingLayer>(
        LayerParams{sizeof(fp32), {24, 24, 64}},
        LayerParams{sizeof(fp32), {12, 12, 64}}
    );
    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {12, 12, 64}},
        LayerParams{sizeof(fp32), {10, 10, 64}},
        LayerParams{sizeof(fp32), {3, 3, 64, 64}, modelPath / "conv5_weights.bin"},
        LayerParams{sizeof(fp32), {64}, modelPath / "conv5_biases.bin"}
    );
    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {10, 10, 64}},
        LayerParams{sizeof(fp32), {8, 8, 128}},
        LayerParams{sizeof(fp32), {3, 3, 64, 128}, modelPath / "conv6_weights.bin"},
        LayerParams{sizeof(fp32), {128}, modelPath / "conv6_biases.bin"}
    );
    model.addLayer<MaxPoolingLayer>(
        LayerParams{sizeof(fp32), {8, 8, 128}},
        LayerParams{sizeof(fp32), {4, 4, 128}}
    );
    model.addLayer<FlattenLayer>(
        LayerParams{sizeof(fp32), {4, 4, 128}},
        LayerParams{sizeof(fp32), {2048}}
    );
    model.addLayer<DenseLayer>(
        LayerParams{sizeof(fp32), {2048}},
        LayerParams{sizeof(fp32), {256}},
        LayerParams{sizeof(fp32), {2048, 256}, modelPath / "dense1_weights.bin"},
        LayerParams{sizeof(fp32), {256}, modelPath / "dense1_biases.bin"}
    );
    model.addLayer<DenseLayer>(
        LayerParams{sizeof(fp32), {256}},
        LayerParams{sizeof(fp32), {200}},
        LayerParams{sizeof(fp32), {256, 200}, modelPath / "dense2_weights.bin"},
        LayerParams{sizeof(fp32), {200}, modelPath / "dense2_biases.bin"},
        false
    );
    model.addLayer<SoftmaxLayer>(
        LayerParams{sizeof(fp32), {200}},
        LayerParams{sizeof(fp32), {200}}
    );

    return model;
}

double benchmarkMode(const Model& model, const LayerData& img, Layer::InfType infType, std::size_t iters) {
    Timer timer("Benchmark");
    timer.start();
    for (std::size_t i = 0; i < iters; ++i) {
        model.inference(img, infType);
    }
    timer.stop();
    // timer.milliseconds is total elapsed time in ms
    return static_cast<double>(timer.milliseconds) / static_cast<double>(iters);
}

void runTests() {
    Path basePath("data");
    Model model = buildToyModel(basePath / "model");
    model.allocLayers();

    LayerData img(model[0].getInputParams(), basePath / "image_0.bin");
    img.loadData();

    LayerData expected(model.getOutputLayer().getOutputParams(), basePath / "image_0_data" / "layer_11_output.bin");
    expected.loadData();

    std::cout << "\n======================================================\n";
    std::cout << "  LAB 6: VERIFYING CORRECTNESS AGAINST GOLDEN OUTPUT\n";
    std::cout << "======================================================\n";
    ConvolutionalLayer::setTileBlockSize(16);
    const LayerData& tiledOut = model.inference(img, Layer::InfType::TILED);
    tiledOut.compareWithinPrint<fp32>(expected);

    std::cout << "\n======================================================\n";
    std::cout << "  LAB 6 BENCHMARK 1: NAIVE vs REORDERED vs TILING\n";
    std::cout << "  (Averaged over " << BENCH_ITERATIONS << " full-model inferences)\n";
    std::cout << "======================================================\n";

    const double naiveMs = benchmarkMode(model, img, Layer::InfType::NAIVE, BENCH_ITERATIONS);
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "[Baseline] Naive (p,q,m,r,s,c)    : " << naiveMs << " ms/img (1.00x)\n";

    // Block size 0 = pure reordered loop (p,q,r,s,c,m) without tiling
    ConvolutionalLayer::setTileBlockSize(0);
    const double reorderedMs = benchmarkMode(model, img, Layer::InfType::TILED, BENCH_ITERATIONS);
    std::cout << "[Reordered] Unit-Stride (m inner) : " << reorderedMs << " ms/img ("
              << (naiveMs / reorderedMs) << "x speedup)\n";

    const size blockSizes[] = {2, 4, 8, 16, 32};
    double tiledMs[5] = {};
    for (int i = 0; i < 5; ++i) {
        ConvolutionalLayer::setTileBlockSize(blockSizes[i]);
        tiledMs[i] = benchmarkMode(model, img, Layer::InfType::TILED, BENCH_ITERATIONS);
        std::cout << "[Tiled] Block Size = " << std::setw(2) << blockSizes[i] << "           : "
                  << tiledMs[i] << " ms/img (" << (naiveMs / tiledMs[i]) << "x speedup)\n";
    }

    const double simdMs = benchmarkMode(model, img, Layer::InfType::SIMD, BENCH_ITERATIONS);
    std::cout << "[SIMD] Vectorized Inner Loop      : " << simdMs << " ms/img ("
              << (naiveMs / simdMs) << "x speedup)\n";

#ifndef ZEDBOARD
    std::cout << "\n======================================================\n";
    std::cout << "  LAB 6 BENCHMARK 2: MULTITHREADING (1 to 64 Threads)\n";
    std::cout << "======================================================\n";
    const size threadCounts[] = {1, 2, 4, 8, 16, 32, 64};
    double threadMs[7] = {};
    for (int i = 0; i < 7; ++i) {
        ConvolutionalLayer::setNumThreads(threadCounts[i]);
        threadMs[i] = benchmarkMode(model, img, Layer::InfType::THREADED, BENCH_ITERATIONS);
        std::cout << "[Threaded] Threads = " << std::setw(2) << threadCounts[i] << "           : "
                  << threadMs[i] << " ms/img (" << (naiveMs / threadMs[i]) << "x speedup, "
                  << (1000.0 / threadMs[i]) << " img/s)\n";
    }

    // Export CSV for automatic plotting script
    std::ofstream csv("lab6_benchmark_results.csv");
    if (csv) {
        csv << "category,parameter,latency_ms,speedup,throughput_ips\n";
        csv << "baseline,naive," << naiveMs << ",1.0," << (1000.0 / naiveMs) << "\n";
        csv << "reordered,unit_stride," << reorderedMs << "," << (naiveMs / reorderedMs) << "," << (1000.0 / reorderedMs) << "\n";
        for (int i = 0; i < 5; ++i) {
            csv << "tiling," << blockSizes[i] << "," << tiledMs[i] << "," << (naiveMs / tiledMs[i]) << "," << (1000.0 / tiledMs[i]) << "\n";
        }
        csv << "simd,vectorized," << simdMs << "," << (naiveMs / simdMs) << "," << (1000.0 / simdMs) << "\n";
        for (int i = 0; i < 7; ++i) {
            csv << "threading," << threadCounts[i] << "," << threadMs[i] << "," << (naiveMs / threadMs[i]) << "," << (1000.0 / threadMs[i]) << "\n";
        }
        std::cout << "\n-> Saved benchmark metrics to lab6_benchmark_results.csv\n";
    }
#endif

    model.freeLayers();
    std::cout << "\n----- ML::runTests() COMPLETE -----\n";
}

} // namespace ML

#ifdef ZEDBOARD
extern "C"
int main() {
    try {
        static FATFS fatfs;
        if (f_mount(&fatfs, "/", 1) != FR_OK) {
            throw std::runtime_error("Failed to mount SD card. Is it plugged in?");
        }
        ML::runTests();
    } catch (const std::exception& e) {
        std::cerr << "\n\n----- EXCEPTION THROWN -----\n" << e.what() << '\n';
    }
    std::cout << "\n\n----- STARTING FILE TRANSFER SERVER -----\n";
    FileServer::start_file_transfer_server();
}
#else
int main() {
    ML::runTests();
}
#endif
