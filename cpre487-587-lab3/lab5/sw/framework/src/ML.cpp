#include <iostream>
#include <sstream>
#include <vector>

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>

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
#include "Mac.h"
#include <algorithm>
#ifdef ZEDBOARD
#include <file_transfer/file_transfer.h>
#endif

namespace ML {

// Test images in the data folder: image_0.bin ... image_2.bin
const std::size_t NUM_TEST_IMAGES = 3;

// The TensorFlow model has 12 layers and its last layer (dense_1) includes the softmax activation. Here that layer is
// split into Dense 2 and Softmax, so layers 0-10 match reference outputs 0-10 one to one, and layers 11 + 12 together
// match reference output 11.
const std::size_t DENSE2_LAYER = 11;
const std::size_t FINAL_REFERENCE = 11;

// Build our ML toy model
Model buildToyModel(const Path modelPath) {
    Model model;
    logInfo("--- Building Toy Model ---");

    // --- Conv 1: L1 ---
    // Input shape: 64x64x3
    // Output shape: 60x60x32

    // You can pick how you want to implement your layers, both are allowed:

    // LayerParams conv1_inDataParam(sizeof(fp32), {64, 64, 3});
    // LayerParams conv1_outDataParam(sizeof(fp32), {60, 60, 32});
    // LayerParams conv1_weightParam(sizeof(fp32), {5, 5, 3, 32}, modelPath / "conv1_weights.bin");
    // LayerParams conv1_biasParam(sizeof(fp32), {32}, modelPath / "conv1_biases.bin");
    // auto conv1 = new ConvolutionalLayer(conv1_inDataParam, conv1_outDataParam, conv1_weightParam, conv1_biasParam);

    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {64, 64, 3}},                                    // Input Data
        LayerParams{sizeof(fp32), {60, 60, 32}},                                   // Output Data
        LayerParams{sizeof(fp32), {5, 5, 3, 32}, modelPath / "conv1_weights.bin"}, // Weights
        LayerParams{sizeof(fp32), {32}, modelPath / "conv1_biases.bin"}            // Bias
    );

    // --- Conv 2: L2 ---
    // Input shape: 60x60x32
    // Output shape: 56x56x32
    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {60, 60, 32}},                                    // Input Data
        LayerParams{sizeof(fp32), {56, 56, 32}},                                    // Output Data
        LayerParams{sizeof(fp32), {5, 5, 32, 32}, modelPath / "conv2_weights.bin"}, // Weights
        LayerParams{sizeof(fp32), {32}, modelPath / "conv2_biases.bin"}             // Bias
    );

    // --- MPL 1: L3 ---
    // Input shape: 56x56x32
    // Output shape: 28x28x32
    model.addLayer<MaxPoolingLayer>(
        LayerParams{sizeof(fp32), {56, 56, 32}},
        LayerParams{sizeof(fp32), {28, 28, 32}}
    );

    // --- Conv 3: L4 ---
    // Input shape: 28x28x32
    // Output shape: 26x26x64
    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {28, 28, 32}},
        LayerParams{sizeof(fp32), {26, 26, 64}},
        LayerParams{sizeof(fp32), {3, 3, 32, 64}, modelPath / "conv3_weights.bin"},
        LayerParams{sizeof(fp32), {64}, modelPath / "conv3_biases.bin"}
    );

    // --- Conv 4: L5 ---
    // Input shape: 26x26x64
    // Output shape: 24x24x64
    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {26, 26, 64}},
        LayerParams{sizeof(fp32), {24, 24, 64}},
        LayerParams{sizeof(fp32), {3, 3, 64, 64}, modelPath / "conv4_weights.bin"},
        LayerParams{sizeof(fp32), {64}, modelPath / "conv4_biases.bin"}
    );

    // --- MPL 2: L6 ---
    // Input shape: 24x24x64
    // Output shape: 12x12x64
    model.addLayer<MaxPoolingLayer>(
        LayerParams{sizeof(fp32), {24, 24, 64}},
        LayerParams{sizeof(fp32), {12, 12, 64}}
    );

    // --- Conv 5: L7 ---
    // Input shape: 12x12x64
    // Output shape: 10x10x64
    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {12, 12, 64}},
        LayerParams{sizeof(fp32), {10, 10, 64}},
        LayerParams{sizeof(fp32), {3, 3, 64, 64}, modelPath / "conv5_weights.bin"},
        LayerParams{sizeof(fp32), {64}, modelPath / "conv5_biases.bin"}
    );

    // --- Conv 6: L8 ---
    // Input shape: 10x10x64
    // Output shape: 8x8x128
    model.addLayer<ConvolutionalLayer>(
        LayerParams{sizeof(fp32), {10, 10, 64}},
        LayerParams{sizeof(fp32), {8, 8, 128}},
        LayerParams{sizeof(fp32), {3, 3, 64, 128}, modelPath / "conv6_weights.bin"},
        LayerParams{sizeof(fp32), {128}, modelPath / "conv6_biases.bin"}
    );

    // --- MPL 3: L9 ---
    // Input shape: 8x8x128
    // Output shape: 4x4x128
    model.addLayer<MaxPoolingLayer>(
        LayerParams{sizeof(fp32), {8, 8, 128}},
        LayerParams{sizeof(fp32), {4, 4, 128}}
    );

    // --- Flatten 1: L10 ---
    // Input shape: 4x4x128
    // Output shape: 2048
    model.addLayer<FlattenLayer>(
        LayerParams{sizeof(fp32), {4, 4, 128}},
        LayerParams{sizeof(fp32), {2048}}
    );

    // --- Dense 1: L11 ---
    // Input shape: 2048
    // Output shape: 256
    model.addLayer<DenseLayer>(
        LayerParams{sizeof(fp32), {2048}},
        LayerParams{sizeof(fp32), {256}},
        LayerParams{sizeof(fp32), {2048, 256}, modelPath / "dense1_weights.bin"},
        LayerParams{sizeof(fp32), {256}, modelPath / "dense1_biases.bin"}
    );

    // --- Dense 2: L12 ---
    // Input shape: 256
    // Output shape: 200
    // No ReLU here: the output of this layer goes to the softmax layer
    model.addLayer<DenseLayer>(
        LayerParams{sizeof(fp32), {256}},
        LayerParams{sizeof(fp32), {200}},
        LayerParams{sizeof(fp32), {256, 200}, modelPath / "dense2_weights.bin"},
        LayerParams{sizeof(fp32), {200}, modelPath / "dense2_biases.bin"},
        false
    );

    // --- Softmax 1: L13 ---
    // Input shape: 200
    // Output shape: 200
    model.addLayer<SoftmaxLayer>(
        LayerParams{sizeof(fp32), {200}},
        LayerParams{sizeof(fp32), {200}}
    );

    return model;
}

void runBasicTest(const Model& model, const Path& basePath) {
    logInfo("--- Running Basic Test ---");

    // Load an image
    LayerData img = {{sizeof(fp32), {64, 64, 3}, "./data/image_0.bin"}};
    img.loadData();

    // Compare images
    std::cout << "Comparing image 0 to itself (max error): " << img.maxDiff<fp32>(img) << std::endl
              << "Comparing image 0 to itself (T/F within epsilon " << ML::Config::EPSILON << "): " << std::boolalpha
              << img.compareWithin<fp32>(img, ML::Config::EPSILON) << std::endl;

    // Test again with a modified copy
    std::cout << "\nChange a value by 0.1 and compare again" << std::endl;
    
    LayerData imgCopy = img;
    imgCopy.get<fp32>(0) += 0.1;

    // Compare images
    img.compareWithinPrint<fp32>(imgCopy);

    // Test again with a modified copy
    log("Change a value by 0.1 and compare again...");
    imgCopy.get<fp32>(0) += 0.1;

    // Compare Images
    img.compareWithinPrint<fp32>(imgCopy);
}

// Path of the TensorFlow reference output of a layer for one of the test images
Path referenceOutputPath(const Path& basePath, const std::size_t imageNum, const std::size_t layerNum) {
    return basePath / ("image_" + std::to_string(imageNum) + "_data") / ("layer_" + std::to_string(layerNum) + "_output.bin");
}

// Test one layer by itself: its input is the TensorFlow output of the previous layer (or the image for layer 0),
// so an error in an earlier layer cannot affect the result
void runLayerTest(const std::size_t layerNum, const Model& model, const Path& basePath) {
    for (std::size_t imageNum = 0; imageNum < NUM_TEST_IMAGES; imageNum++) {
        logInfo(std::string("--- Running Layer Test ") + std::to_string(layerNum) + ", image " + std::to_string(imageNum) + " ---");

        const Path inputPath = (layerNum == 0) ? basePath / ("image_" + std::to_string(imageNum) + ".bin")
                                               : referenceOutputPath(basePath, imageNum, layerNum - 1);
        LayerData input(model[layerNum].getInputParams(), inputPath);
        input.loadData();

        Timer timer("Layer Inference");

        // Run inference on the layer
        timer.start();
        const LayerData& output = model.inferenceLayer(input, layerNum, Layer::InfType::NAIVE);
        timer.stop();

        // Compare the output
        LayerData expected(output.getParams(), referenceOutputPath(basePath, imageNum, layerNum));
        expected.loadData();
        output.compareWithinPrint<fp32>(expected);
    }
}

// Test Dense 2 and Softmax together, because TensorFlow has no separate output for Dense 2:
// the input is the reference output of Dense 1, and the result is compared with the final reference output
void runOutputStageTest(const Model& model, const Path& basePath) {
    for (std::size_t imageNum = 0; imageNum < NUM_TEST_IMAGES; imageNum++) {
        logInfo(std::string("--- Running Layer Test ") + std::to_string(DENSE2_LAYER) + " + " + std::to_string(DENSE2_LAYER + 1) +
                ", image " + std::to_string(imageNum) + " ---");

        LayerData input(model[DENSE2_LAYER].getInputParams(), referenceOutputPath(basePath, imageNum, DENSE2_LAYER - 1));
        input.loadData();

        Timer timer("Layer Inference");

        timer.start();
        const LayerData& logits = model.inferenceLayer(input, DENSE2_LAYER, Layer::InfType::NAIVE);
        const LayerData& output = model.inferenceLayer(logits, DENSE2_LAYER + 1, Layer::InfType::NAIVE);
        timer.stop();

        LayerData expected(output.getParams(), referenceOutputPath(basePath, imageNum, FINAL_REFERENCE));
        expected.loadData();
        output.compareWithinPrint<fp32>(expected);
    }
}
#ifndef ZEDBOARD
static void saveConv1Channel0(const LayerData& output, const std::string& filename,
                              bool quantized, fp32 scale = 1.0f,
                              i32 zeroPoint = 0) {
    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("Could not open " + filename);
    }

    for (size p = 0; p < 60; ++p) {
        for (size q = 0; q < 60; ++q) {
            const size index = (p * 60 + q) * 32;
            const fp32 value = quantized
                ? (static_cast<fp32>(output.get<i8>(index)) - zeroPoint) / scale
                : output.get<fp32>(index);

            if (q != 0) file << ',';
            file << value;
        }
        file << '\n';
    }
}
#endif

#ifndef ZEDBOARD
static void evaluateValidation(const Model& model, int bits,
                               fp32 inputScale = 1.0f,
                               i32 inputZeroPoint = 0) {
    std::ifstream labels("data/validation/labels.csv");
    if (!labels) {
        throw std::runtime_error("Could not open data/validation/labels.csv");
    }

    const bool quantized = bits != 32;
    const i32 quantMin = quantized ? -(1 << (bits - 1)) : 0;
    const i32 quantMax = quantized ? (1 << (bits - 1)) - 1 : 0;

    LayerData quantizedInput(model[0].getInputParams());
    if (quantized) {
        quantizedInput.allocData();
    }

    size total = 0;
    size top1 = 0;
    size top10 = 0;
    std::string line;
    std::getline(labels, line);  // Skip the filename,label header.

    while (std::getline(labels, line)) {
        std::istringstream row(line);
        std::string filename, labelText;
        if (!std::getline(row, filename, ',') ||
            !std::getline(row, labelText) || filename.empty()) {
            throw std::runtime_error("Invalid validation label row: " + line);
        }
        const int label = std::stoi(labelText);

        const Path imagePath = Path("data/validation") / std::string(filename);
        LayerData image(LayerParams{sizeof(fp32), {64, 64, 3}, imagePath});
        image.loadData();

        const LayerData* input = &image;
        if (quantized) {
            for (size i = 0; i < image.getParams().flat_count(); ++i) {
                fp32 value = std::round(image.get<fp32>(i) * inputScale)
                           + inputZeroPoint;
                if (value < quantMin) value = quantMin;
                if (value > quantMax) value = quantMax;
                quantizedInput.get<i8>(i) = static_cast<i8>(value);
            }
            input = &quantizedInput;
        }

        const LayerData& output = model.inference(*input, Layer::InfType::NAIVE);
        const size classes = output.getParams().flat_count();
        if (classes < 10 || label < 0 || static_cast<size>(label) >= classes) {
            throw std::runtime_error("Invalid class label for " + filename);
        }

        std::vector<size> ranked(classes);
        for (size i = 0; i < classes; ++i) {
            ranked[i] = i;
        }

        // Rank higher scores first. Break ties by smaller class index.
        std::partial_sort(ranked.begin(), ranked.begin() + 10, ranked.end(),
            [&](size a, size b) {
                const fp32 scoreA = output.get<fp32>(a);
                const fp32 scoreB = output.get<fp32>(b);
                return scoreA == scoreB ? a < b : scoreA > scoreB;
            });

        const size target = static_cast<size>(label);
        if (ranked.front() == target) ++top1;
        if (std::find(ranked.begin(), ranked.begin() + 10, target)
            != ranked.begin() + 10) {
            ++top10;
        }
        ++total;
    }

    if (total != 1000) {
        throw std::runtime_error("Expected 1000 validation images, got "
                                 + std::to_string(total));
    }

    const std::string name = quantized
        ? "INT" + std::to_string(bits) : "FP32";

    std::cout << "Validation " << name
              << " | Top-1: " << top1 << '/' << total
              << " (" << 100.0 * top1 / total << "%)"
              << " | Top-10: " << top10 << '/' << total
              << " (" << 100.0 * top10 / total << "%)\n";
}
#endif

void runInferenceTest(const Model& model, const Path& basePath) {
    for (std::size_t imageNum = 0; imageNum < NUM_TEST_IMAGES; imageNum++) {
        logInfo(std::string("--- Running Inference Test, image ") + std::to_string(imageNum) + " ---");

        LayerData img(model[0].getInputParams(), basePath / ("image_" + std::to_string(imageNum) + ".bin"));
        img.loadData();

        Timer timer("Full Inference");

        // Run inference on the model
        timer.start();
        const LayerData& output = model.inference(img, Layer::InfType::NAIVE);
        timer.stop();

        // Compare the output with the final TensorFlow output
        LayerData expected(model.getOutputLayer().getOutputParams(), referenceOutputPath(basePath, imageNum, FINAL_REFERENCE));
        expected.loadData();
        output.compareWithinPrint<fp32>(expected);

        // Report the predicted class (index of the largest output) and its confidence
        std::size_t best = 0;
        for (std::size_t i = 1; i < output.getParams().flat_count(); i++) {
            if (output.get<fp32>(i) > output.get<fp32>(best)) best = i;
        }
        std::cout << "Predicted class: " << best << " (confidence " << output.get<fp32>(best) << ")" << std::endl;
        #ifndef ZEDBOARD
        if (imageNum == 0) {
            const LayerData& conv1Output =
                model.inferenceLayer(img, 0, Layer::InfType::NAIVE);
            saveConv1Channel0(conv1Output, "data/conv1_ch0_fp32.csv", false);
        }
#endif
    }
}

static std::string readTextFile(const std::string& path) {
#ifdef ZEDBOARD
    FIL file;
    if (f_open(&file, path.c_str(), FA_OPEN_EXISTING | FA_READ) != FR_OK) {
        throw std::runtime_error("Could not open " + path);
    }
    std::string text;
    char buffer[256];
    UINT bytesRead = 0;
    while (f_read(&file, buffer, sizeof(buffer), &bytesRead) == FR_OK && bytesRead > 0) {
        text.append(buffer, bytesRead);
    }
    f_close(&file);
    return text;
#else
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Could not open " + path);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
#endif
}


static void testQuantizedModel(int bits) {
    const std::string folder = "data/model_int" + std::to_string(bits);
    const Path modelPath(folder.c_str());
    std::istringstream csv(readTextFile(folder + "/quantization.csv"));
    const i32 quantMin = -(1 << (bits - 1));
    const i32 quantMax = (1 << (bits - 1)) - 1;

    struct QuantParams {
        fp32 inputScale;
        fp32 weightScale;
        i32 inputZeroPoint;
    };

    const char* names[] = {
        "conv1", "conv2", "conv3", "conv4",
        "conv5", "conv6", "dense1", "dense2"
    };
    QuantParams settings[8] = {};

    std::string line;
    std::getline(csv, line);  // Skip the header.

    while (std::getline(csv, line)) {
        std::istringstream row(line);
        std::string name, si, sw, sb, zi;

        if (!std::getline(row, name, ',') ||
            !std::getline(row, si, ',') ||
            !std::getline(row, sw, ',') ||
            !std::getline(row, sb, ',') ||
            !std::getline(row, zi)) {
            continue;
        }

        for (size i = 0; i < 8; ++i) {
            if (name == names[i]) {
                settings[i] = QuantParams{
                    std::stof(si), std::stof(sw), std::stoi(zi)
                };
                break;
            }
        }
    }

    for (size i = 0; i < 8; ++i) {
        if (!(settings[i].inputScale > 0.0f) ||
            !(settings[i].weightScale > 0.0f)) {
            throw std::runtime_error(
                std::string("Missing or invalid quantization settings for ") +
                names[i]);
        }
    }

    Model model;

    const auto addConv = [&](size index, const dimVec& inputDims,
                             const dimVec& outputDims, const dimVec& weightDims) {
        const auto& current = settings[index];
        const auto& next = settings[index + 1];
        const std::string name = names[index];

        model.addLayer<ConvolutionalLayer>(
            LayerParams{sizeof(i8), inputDims},
            LayerParams{sizeof(i8), outputDims},
            LayerParams{sizeof(i8), weightDims, modelPath / (name + "_weights.bin")},
            LayerParams{sizeof(i32), {outputDims[2]}, modelPath / (name + "_biases.bin")},
            current.inputScale, current.weightScale, current.inputZeroPoint,
            next.inputScale, next.inputZeroPoint,bits
        );
    };

    const auto addPool = [&](const dimVec& inputDims, const dimVec& outputDims) {
        model.addLayer<MaxPoolingLayer>(
            LayerParams{sizeof(i8), inputDims},
            LayerParams{sizeof(i8), outputDims},
            true
        );
    };

    addConv(0, {64, 64, 3},   {60, 60, 32},  {5, 5, 3, 32});
    addConv(1, {60, 60, 32},  {56, 56, 32},  {5, 5, 32, 32});
    addPool({56, 56, 32}, {28, 28, 32});
    addConv(2, {28, 28, 32},  {26, 26, 64},  {3, 3, 32, 64});
    addConv(3, {26, 26, 64},  {24, 24, 64},  {3, 3, 64, 64});
    addPool({24, 24, 64}, {12, 12, 64});
    addConv(4, {12, 12, 64},  {10, 10, 64},  {3, 3, 64, 64});
    addConv(5, {10, 10, 64},  {8, 8, 128},   {3, 3, 64, 128});
    addPool({8, 8, 128}, {4, 4, 128});

    model.addLayer<FlattenLayer>(
        LayerParams{sizeof(i8), {4, 4, 128}},
        LayerParams{sizeof(i8), {2048}}
    );

    const auto& dense1 = settings[6];
    const auto& dense2 = settings[7];

    model.addLayer<DenseLayer>(
        LayerParams{sizeof(i8), {2048}},
        LayerParams{sizeof(i8), {256}},
        LayerParams{sizeof(i8), {2048, 256}, modelPath / "dense1_weights.bin"},
        LayerParams{sizeof(i32), {256}, modelPath / "dense1_biases.bin"},
        true, dense1.inputScale, dense1.weightScale, dense1.inputZeroPoint,
        dense2.inputScale, dense2.inputZeroPoint,bits
    );

    model.addLayer<DenseLayer>(
        LayerParams{sizeof(i8), {256}},
        LayerParams{sizeof(fp32), {200}},
        LayerParams{sizeof(i8), {256, 200}, modelPath / "dense2_weights.bin"},
        LayerParams{sizeof(i32), {200}, modelPath / "dense2_biases.bin"},
        false, dense2.inputScale, dense2.weightScale, dense2.inputZeroPoint
    );

    model.addLayer<SoftmaxLayer>(
        LayerParams{sizeof(fp32), {200}},
        LayerParams{sizeof(fp32), {200}}
    );

       model.allocLayers();
    const auto& conv1 = settings[0];

    for (size imageNum = 0; imageNum < NUM_TEST_IMAGES; ++imageNum) {
        const std::string imageName =
            "image_" + std::to_string(imageNum);
        const Path imagePath = Path("data") / (imageName + ".bin");

        LayerData image(LayerParams{sizeof(fp32), {64, 64, 3}, imagePath});
        image.loadData();

        LayerData inputInt8(model[0].getInputParams());
        inputInt8.allocData();

        for (size i = 0; i < 64 * 64 * 3; ++i) {
            fp32 value = std::round(image.get<fp32>(i) * conv1.inputScale)
                       + conv1.inputZeroPoint;
            if (value < quantMin) value = quantMin;
            if (value > quantMax) value = quantMax;
            inputInt8.get<i8>(i) = static_cast<i8>(value);
        }

        std::cout << "INT" << bits << ' ' << imageName << '\n';

        Timer timer("Full Inference");
        timer.start();
        const LayerData& output = model.inference(inputInt8, Layer::InfType::NAIVE);
        timer.stop();

        size best = 0;
        for (size i = 1; i < output.getParams().flat_count(); ++i) {
            if (output.get<fp32>(i) > output.get<fp32>(best)) best = i;
        }

        std::cout << "INT" << bits << ' ' << imageName
                  << " predicted class: " << best
                  << " (confidence " << output.get<fp32>(best) << ")\n";

        // ---- ACCELERATED vs NAIVE 비교 ----
        std::vector<fp32> naiveOut(output.getParams().flat_count());
        for (size i = 0; i < naiveOut.size(); ++i) naiveOut[i] = output.get<fp32>(i);

        Timer accTimer("Accelerated Inference");
        accTimer.start();
        const LayerData& accOut = model.inference(inputInt8, Layer::InfType::ACCELERATED);
        accTimer.stop();

        size mismatches = 0;
        for (size i = 0; i < naiveOut.size(); ++i) {
            if (accOut.get<fp32>(i) != naiveOut[i]) ++mismatches;
        }
        std::cout << "INT" << bits << ' ' << imageName
                  << " ACCEL vs NAIVE mismatches: " << mismatches << " / " << naiveOut.size() << "\n";
        #ifndef ZEDBOARD
        if (imageNum == 0) {
            const LayerData& conv1Output =
                model.inferenceLayer(inputInt8, 0, Layer::InfType::NAIVE);
            saveConv1Channel0(
                conv1Output,
                "data/conv1_ch0_int" + std::to_string(bits) + ".csv",
                true, settings[1].inputScale, settings[1].inputZeroPoint
            );
        }
        #endif
    }
    #ifndef ZEDBOARD
    evaluateValidation(model, bits, conv1.inputScale, conv1.inputZeroPoint);
    #endif
    model.freeLayers();
}


void runTests() {
    // Base input data path (determined from current directory of where you are running the command)
    Path basePath("data");  // May need to be altered for zedboards loading from SD Cards
    #ifdef ZEDBOARD
        testQuantizedModel(MAC_BITS);
    #else
        testQuantizedModel(8);
        testQuantizedModel(4);
        testQuantizedModel(2);
    #endif

    // Build the model and allocate the buffers
    Model model = buildToyModel(basePath / "model");
    model.allocLayers();

    // Run some framework tests as an example of loading data
    runBasicTest(model, basePath);

    // Run a layer inference test for every layer that has its own reference output, then Dense 2 + Softmax together
    for (std::size_t layerNum = 0; layerNum < DENSE2_LAYER; layerNum++) {
        runLayerTest(layerNum, model, basePath);
    }
    runOutputStageTest(model, basePath);

    // Run an end-to-end inference test
    runInferenceTest(model, basePath);

    #ifndef ZEDBOARD
    evaluateValidation(model, 32);
    #endif

    // Clean up
    model.freeLayers();
    std::cout << "\n\n----- ML::runTests() COMPLETE -----\n";
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