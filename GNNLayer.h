#ifndef GNNLAYER_H
#define GNNLAYER_H

#include <vector>
#include <cmath>
#include <random>

class GNNLayer {
public:
    int inFeatures, outFeatures;
    std::vector<std::vector<double>> W_self;  // [out][in]
    std::vector<std::vector<double>> W_neigh; // [out][in]
    std::vector<double> bias;

    GNNLayer(int inF, int outF) : inFeatures(inF), outFeatures(outF) {
        std::random_device rd;
        std::mt19937 rng(rd());
        std::normal_distribution<double> dist(0.0, 0.1);
        W_self.resize(outF, std::vector<double>(inF));
        W_neigh.resize(outF, std::vector<double>(inF));
        bias.resize(outF);
        for (int i = 0; i < outF; ++i) {
            bias[i] = dist(rng);
            for (int j = 0; j < inF; ++j) {
                W_self[i][j] = dist(rng);
                W_neigh[i][j] = dist(rng);
            }
        }
    }

    // forward: node features (F) + neighbor features matrix (M x F) -> output (F')
    std::vector<double> forward(const std::vector<double>& selfFeat,
                                const std::vector<std::vector<double>>& neighFeats) {
        int F = inFeatures;
        int Fout = outFeatures;
        std::vector<double> out(Fout, 0.0);
        // self
        for (int i = 0; i < Fout; ++i) {
            double val = 0.0;
            for (int j = 0; j < F; ++j) val += W_self[i][j] * selfFeat[j];
            out[i] = val;
        }
        // neighbors (mean aggregation)
        int M = neighFeats.size();
        if (M > 0) {
            std::vector<double> neighMean(Fout, 0.0);
            for (int i = 0; i < Fout; ++i) {
                double sum = 0.0;
                for (int k = 0; k < M; ++k) {
                    for (int j = 0; j < F; ++j) sum += W_neigh[i][j] * neighFeats[k][j];
                }
                neighMean[i] = sum / M;
            }
            for (int i = 0; i < Fout; ++i) out[i] += neighMean[i];
        }
        // bias + relu
        for (int i = 0; i < Fout; ++i) {
            out[i] += bias[i];
            out[i] = out[i] > 0 ? out[i] : 0;  // ReLU
        }
        return out;
    }

    // For FL: get all weights as a flat vector
    std::vector<double> getFlatWeights() const {
        std::vector<double> flat;
        for (auto& row : W_self) for (double v : row) flat.push_back(v);
        for (auto& row : W_neigh) for (double v : row) flat.push_back(v);
        for (double b : bias) flat.push_back(b);
        return flat;
    }

    // Set weights from flat vector
    void setFlatWeights(const std::vector<double>& flat, size_t& idx) {
        for (auto& row : W_self) for (double& v : row) v = flat[idx++];
        for (auto& row : W_neigh) for (double& v : row) v = flat[idx++];
        for (double& b : bias) b = flat[idx++];
    }

    // Return number of parameters
    size_t numParams() const {
        return W_self.size()*inFeatures + W_neigh.size()*inFeatures + bias.size();
    }
};

#endif
