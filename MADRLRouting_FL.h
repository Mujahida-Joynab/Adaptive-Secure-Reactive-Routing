#ifndef MADRLROUTING_FL_H
#define MADRLROUTING_FL_H

#include <omnetpp.h>
#include <map>
#include <vector>
#include <deque>
#include <algorithm>
#include <random>
#include "QRoutingMessages_m.h"
#include "GNNPolicy.h"

using namespace omnetpp;

struct Experience {
    std::vector<double> state;    // concatenated self features
    std::vector<std::vector<double>> neighStates; // neighbor features
    int action;                   // neighbor index
    double reward;
    std::vector<double> nextSelfState;
    std::vector<std::vector<double>> nextNeighStates;
    bool done;
};

class MADRLRouting_FL : public cSimpleModule
{
protected:
    int myId;
    double alpha;
    double updateInterval;
    double discountFactor;
    int replayCapacity;
    int batchSize;
    int hiddenDim;
    double epsilon;
    simtime_t flInterval;

    // neighbor info
    struct NeighborInfo {
        int id;
        double x, y;
        // additional features (fake for now)
        double battery;
        double queueLen;
        double rssi;
        int gateIndex;
    };
    std::map<int, NeighborInfo> neighbors;
    std::vector<int> neighborList;   // ordered for action indexing

    static const int MAX_NEIGHBORS = 10;
    static const int FEAT_DIM = 5;   // x, y, battery, queue, rssi

    // GNN policy networks
    GNNPolicy *qNetwork;
    GNNPolicy *targetNetwork;

    // replay buffer
    std::deque<Experience> replayBuffer;
    int trainStepCount = 0;
    static const int TARGET_UPDATE_FREQ = 50;

    // FL related
    cMessage *flTimer;
    std::map<int, std::vector<double>> neighborModels; // neighborId -> flat weights
    void sendFLModel();
    void aggregateFL();

    // helpers
    std::vector<double> buildSelfFeatures();
    std::vector<std::vector<double>> buildNeighborFeatures();
    int selectAction(const std::vector<double>& selfFeat,
                     const std::vector<std::vector<double>>& neighFeats);
    void storeExperience(const Experience& exp);
    void train();
    void sendUpdate();
    void processHello(QRoutingHello *hello, int gateIndex);
    void processUpdate(QRoutingUpdate *update, int gateIndex);
    void processFLModel(FLModel *msg);
    void forwardData(QData *data);

    cMessage *timer;

public:
    MADRLRouting_FL();
    virtual ~MADRLRouting_FL();
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif
