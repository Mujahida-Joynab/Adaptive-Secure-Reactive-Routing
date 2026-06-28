#include <omnetpp.h>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <algorithm>
#include <random>
#include "QRoutingMessages_m.h"
#include "NeuralNet.h"

using namespace omnetpp;

struct Exp { std::vector<double> state; int action; double reward; std::vector<double> nextState; bool done; };

class MADQNRouting : public cSimpleModule {
  private:
    int myId;
    double alpha, updateInterval, discountFactor, epsilon;
    int replayCapacity, batchSize, hiddenSize;
    struct NeighborInfo { int id; double x,y; int gateIndex; };
    std::map<int, NeighborInfo> neighbors;
    std::vector<int> neighborList;
    static const int MAX_NEIGHBORS = 10;
    static const int FEAT_DIM = 5;
    NeuralNet *qNetwork, *targetNetwork;
    std::deque<Exp> replay;
    int trainStep = 0;
    static const int TARGET_UPDATE = 50;
    cMessage *timer;
    std::vector<double> buildState();
    int selectAction(const std::vector<double>& state);
    void storeExperience(const Exp& e);
    void train();
    void sendUpdate();
    void forwardData(QData *data);
  public:
    MADQNRouting();
    ~MADQNRouting();
  protected:
    void initialize() override;
    void handleMessage(cMessage *msg) override;
    void finish() override;
};

Define_Module(MADQNRouting);

MADQNRouting::MADQNRouting() : qNetwork(nullptr), targetNetwork(nullptr) {}
MADQNRouting::~MADQNRouting() { delete qNetwork; delete targetNetwork; }

void MADQNRouting::initialize() {
    myId = par("nodeId");
    alpha = par("alpha");
    updateInterval = par("updateInterval");
    discountFactor = par("discountFactor");
    epsilon = par("epsilon");
    replayCapacity = par("replayCapacity");
    batchSize = par("batchSize");
    hiddenSize = par("hiddenSize");
    int stateSize = FEAT_DIM + MAX_NEIGHBORS * FEAT_DIM;
    std::vector<int> layers = {stateSize, hiddenSize, MAX_NEIGHBORS};
    qNetwork = new NeuralNet(layers, alpha);
    targetNetwork = new NeuralNet(layers, alpha);
    targetNetwork->copyWeights(*qNetwork);
    cModule *parent = getParentModule();
    double myX = atof(parent->getDisplayString().getTagArg("p", 0));
    double myY = atof(parent->getDisplayString().getTagArg("p", 1));
    for (int i = 0; i < gateSize("out"); i++) {
        QRoutingHello *hello = new QRoutingHello("hello");
        hello->setSrcNodeId(myId);
        hello->setX(myX);
        hello->setY(myY);
        send(hello, "out", i);
    }
    timer = new cMessage("timer");
    scheduleAt(simTime() + uniform(0, updateInterval), timer);
}

void MADQNRouting::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        sendUpdate();
        train();
        scheduleAt(simTime() + updateInterval, timer);
    } else if (auto *hello = dynamic_cast<QRoutingHello*>(msg)) {
        int gateIndex = msg->getArrivalGate()->getIndex();
        int nbId = hello->getSrcNodeId();
        NeighborInfo ni;
        ni.id = nbId; ni.x = hello->getX(); ni.y = hello->getY(); ni.gateIndex = gateIndex;
        neighbors[nbId] = ni;
        if (std::find(neighborList.begin(), neighborList.end(), nbId) == neighborList.end()
            && neighborList.size() < MAX_NEIGHBORS)
            neighborList.push_back(nbId);
        delete msg;
    } else if (auto *update = dynamic_cast<QRoutingUpdate*>(msg)) {
        delete msg;
    } else if (auto *data = dynamic_cast<QData*>(msg)) {
        if (data->getDestId() == myId) send(data, "toApp");
        else forwardData(data);
    } else { delete msg; }
}

std::vector<double> MADQNRouting::buildState() {
    std::vector<double> state(FEAT_DIM + MAX_NEIGHBORS * FEAT_DIM, 0.0);
    cModule *parent = getParentModule();
    state[0] = atof(parent->getDisplayString().getTagArg("p", 0)) / 1500.0;
    state[1] = atof(parent->getDisplayString().getTagArg("p", 1)) / 1500.0;
    state[2] = 1.0; state[3] = 0.0; state[4] = 0.0;
    for (int i = 0; i < MAX_NEIGHBORS; i++) {
        if (i < (int)neighborList.size()) {
            auto it = neighbors.find(neighborList[i]);
            if (it != neighbors.end()) {
                int base = FEAT_DIM + i*FEAT_DIM;
                state[base+0] = it->second.x / 1500.0;
                state[base+1] = it->second.y / 1500.0;
                state[base+2] = 0.5; state[base+3] = 0.0; state[base+4] = 0.0;
            }
        }
    }
    return state;
}

int MADQNRouting::selectAction(const std::vector<double>& state) {
    if (neighborList.empty()) return -1;
    if (uniform(0,1) < epsilon) return intrand(neighborList.size());
    std::vector<double> q = qNetwork->forward(state);
    int best = 0;
    for (int i=1; i<(int)neighborList.size(); i++) if (q[i] > q[best]) best = i;
    return best;
}

void MADQNRouting::forwardData(QData *data) {
    if (data->getTtl() <= 0) { delete data; return; }
    std::vector<double> state = buildState();
    int action = selectAction(state);
    if (action < 0 || action >= (int)neighborList.size()) { delete data; return; }
    int nextHop = neighborList[action];
    int gateIdx = neighbors[nextHop].gateIndex;
    double reward = -1.0;
    std::vector<double> nextState = buildState();
    Exp exp;
    exp.state = state; exp.action = action; exp.reward = reward;
    exp.nextState = nextState; exp.done = (nextHop == data->getDestId());
    storeExperience(exp);
    data->setHopCount(data->getHopCount()+1);
    data->setTtl(data->getTtl()-1);
    sendDelayed(data, 0.01, "out", gateIdx);
}

void MADQNRouting::storeExperience(const Exp& e) {
    if (replay.size() >= (size_t)replayCapacity) replay.pop_front();
    replay.push_back(e);
}

void MADQNRouting::train() {
    if (replay.size() < (size_t)batchSize) return;
    std::vector<int> idx(replay.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), std::mt19937(std::random_device{}()));
    for (int i=0; i<batchSize; i++) {
        Exp &e = replay[idx[i]];
        std::vector<double> qOnline = qNetwork->forward(e.state);
        std::vector<double> qTargetVal = targetNetwork->forward(e.nextState);
        double maxNext = *std::max_element(qTargetVal.begin(), qTargetVal.end());
        double target = e.reward + (e.done ? 0 : discountFactor * maxNext);
        std::vector<double> targetVec = qOnline;
        if (e.action < (int)targetVec.size()) targetVec[e.action] = target;
        qNetwork->backward(e.state, targetVec);
    }
    if (++trainStep % TARGET_UPDATE == 0) targetNetwork->copyWeights(*qNetwork);
}

void MADQNRouting::sendUpdate() {
    QRoutingUpdate *upd = new QRoutingUpdate("upd");
    upd->setSrcNodeId(myId);
    upd->setDestIdArraySize(1); upd->setMinQArraySize(1);
    upd->setDestId(0, myId); upd->setMinQ(0, 0.0);
    for (int i=0; i<gateSize("out"); i++) send(upd->dup(), "out", i);
    delete upd;
}

void MADQNRouting::finish() { cancelAndDelete(timer); }
