#ifndef NEURALNET_H
#define NEURALNET_H

#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

class NeuralNet {
public:
    struct Layer {
        std::vector<std::vector<double>> weights; // [out][in]
        std::vector<double> biases;
        std::vector<double> output;  // cached after forward
        std::vector<double> delta;   // for backprop
    };

    std::vector<Layer> layers;   // public for GNNPolicy to access

private:
    double learningRate;
    std::mt19937 rng;

    static double relu(double x) { return x > 0 ? x : 0; }
    static double reluDeriv(double x) { return x > 0 ? 1 : 0; }

public:
    // layerSizes: e.g., {stateSize, hiddenSize, outputSize}
    NeuralNet(const std::vector<int>& layerSizes, double lr = 0.001) : learningRate(lr) {
        std::random_device rd;
        rng.seed(rd());
        std::normal_distribution<double> dist(0.0, 0.1);
        for (size_t i = 1; i < layerSizes.size(); ++i) {
            Layer l;
            int inSize = layerSizes[i-1];
            int outSize = layerSizes[i];
            l.weights.resize(outSize, std::vector<double>(inSize));
            l.biases.resize(outSize);
            for (int o = 0; o < outSize; ++o) {
                l.biases[o] = dist(rng);
                for (int in = 0; in < inSize; ++in) {
                    l.weights[o][in] = dist(rng);
                }
            }
            l.output.resize(outSize);
            l.delta.resize(outSize);
            layers.push_back(l);
        }
    }

    // Forward pass: returns output vector
    std::vector<double> forward(const std::vector<double>& input) {
        std::vector<double> current = input;
        for (size_t i = 0; i < layers.size(); ++i) {
            std::vector<double> next(layers[i].biases.size());
            for (size_t o = 0; o < next.size(); ++o) {
                double sum = layers[i].biases[o];
                for (size_t in = 0; in < current.size(); ++in)
                    sum += layers[i].weights[o][in] * current[in];
                next[o] = (i == layers.size()-1) ? sum : relu(sum);
            }
            layers[i].output = next;
            current = next;
        }
        return current;
    }

    // Backward pass for MSE loss: target is the desired output vector
    void backward(const std::vector<double>& input, const std::vector<double>& target) {
        // forward to cache outputs
        forward(input);

        // output layer delta = (output - target)
        Layer& last = layers.back();
        for (size_t o = 0; o < last.output.size(); ++o) {
            last.delta[o] = (last.output[o] - target[o]); // MSE derivative
        }

        // backprop through hidden layers
        for (int i = layers.size()-2; i >= 0; --i) {
            Layer& cur = layers[i];
            Layer& next = layers[i+1];
            for (size_t j = 0; j < cur.output.size(); ++j) {
                double error = 0.0;
                for (size_t k = 0; k < next.delta.size(); ++k) {
                    error += next.weights[k][j] * next.delta[k];
                }
                cur.delta[j] = error * reluDeriv(cur.output[j]);
            }
        }

        // update weights & biases
        std::vector<double> layerInput = input;
        for (size_t i = 0; i < layers.size(); ++i) {
            Layer& l = layers[i];
            for (size_t o = 0; o < l.biases.size(); ++o) {
                l.biases[o] -= learningRate * l.delta[o];
                for (size_t in = 0; in < layerInput.size(); ++in) {
                    l.weights[o][in] -= learningRate * l.delta[o] * layerInput[in];
                }
            }
            layerInput = l.output; // input to next layer
        }
    }

    // Copy weights from another network (for target network update)
    void copyWeights(const NeuralNet& other) {
        for (size_t i = 0; i < layers.size(); ++i) {
            layers[i].weights = other.layers[i].weights;
            layers[i].biases = other.layers[i].biases;
        }
    }
};

#endif
