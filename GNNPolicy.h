#ifndef GNNPOLICY_H
#define GNNPOLICY_H

#include "GNNLayer.h"
#include "NeuralNet.h"
#include <vector>

class GNNPolicy {
public:
    int nodeFeatDim;
    int hiddenDim;
    int numNeighbors;
    GNNLayer gnn1, gnn2;
    NeuralNet mlp;   // input: hiddenDim, output: numNeighbors

    GNNPolicy(int featDim, int hidden, int maxNeigh, double lr = 0.001)
        : nodeFeatDim(featDim), hiddenDim(hidden), numNeighbors(maxNeigh),
          gnn1(featDim, hidden), gnn2(hidden, hidden),
          mlp({hidden, maxNeigh}, lr) {}

    std::vector<double> forward(const std::vector<double>& selfFeat,
                                const std::vector<std::vector<double>>& neighFeats) {
        std::vector<double> h1 = gnn1.forward(selfFeat, neighFeats);
        std::vector<double> h2 = gnn2.forward(h1, neighFeats);
        return mlp.forward(h2);
    }

    std::vector<double> getFlatWeights() const {
        auto flat1 = gnn1.getFlatWeights();
        auto flat2 = gnn2.getFlatWeights();
        // MLP weights
        std::vector<double> mlpFlat;
        for (auto& layer : mlp.layers) {
            for (auto& row : layer.weights) for (double v : row) mlpFlat.push_back(v);
            for (double b : layer.biases) mlpFlat.push_back(b);
        }
        flat1.insert(flat1.end(), flat2.begin(), flat2.end());
        flat1.insert(flat1.end(), mlpFlat.begin(), mlpFlat.end());
        return flat1;
    }

    void setFlatWeights(const std::vector<double>& flat) {
        size_t idx = 0;
        gnn1.setFlatWeights(flat, idx);
        gnn2.setFlatWeights(flat, idx);
        for (auto& layer : mlp.layers) {
            for (auto& row : layer.weights) for (double& v : row) v = flat[idx++];
            for (double& b : layer.biases) b = flat[idx++];
        }
    }

    size_t totalParams() const {
        return gnn1.numParams() + gnn2.numParams() + mlp_total_params();
    }

private:
    size_t mlp_total_params() const {
        size_t count = 0;
        for (auto& layer : mlp.layers) {
            count += layer.weights.size() * (layer.weights.empty() ? 0 : layer.weights[0].size());
            count += layer.biases.size();
        }
        return count;
    }
};

#endif
