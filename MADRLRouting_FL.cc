#include "MADRLRouting_FL.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <random>

Define_Module(MADRLRouting_FL);

MADRLRouting_FL::MADRLRouting_FL() : qNetwork(nullptr), targetNetwork(nullptr) {}
MADRLRouting_FL::~MADRLRouting_FL()
{
    delete qNetwork;
    delete targetNetwork;
}

void MADRLRouting_FL::initialize()
{
    myId = par("nodeId");
    alpha = par("alpha");
    updateInterval = par("updateInterval");
    discountFactor = par("discountFactor");
    replayCapacity = par("replayCapacity");
    batchSize = par("batchSize");
    hiddenDim = par("hiddenDim");
    epsilon = par("epsilon");
    flInterval = par("flInterval");

    // Initialise GNN policy networks (one online, one target)
    qNetwork = new GNNPolicy(FEAT_DIM, hiddenDim, MAX_NEIGHBORS, alpha);
    targetNetwork = new GNNPolicy(FEAT_DIM, hiddenDim, MAX_NEIGHBORS, alpha);
    targetNetwork->setFlatWeights(qNetwork->getFlatWeights());

    // Read own position from parent module's display string
    cModule *parent = getParentModule();
    double myX = atof(parent->getDisplayString().getTagArg("p", 0));
    double myY = atof(parent->getDisplayString().getTagArg("p", 1));

    // Send hello on every output gate
    for (int i = 0; i < gateSize("out"); i++) {
        QRoutingHello *hello = new QRoutingHello("hello");
        hello->setSrcNodeId(myId);
        hello->setX(myX);
        hello->setY(myY);
        send(hello, "out", i);
    }

    // Schedule periodic Q‑update broadcasting
    timer = new cMessage("updateTimer");
    scheduleAt(simTime() + uniform(0, updateInterval), timer);

    // Schedule first FL aggregation
    flTimer = new cMessage("flTimer");
    scheduleAt(simTime() + flInterval, flTimer);

    WATCH(myId);
}

void MADRLRouting_FL::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == timer) {
            sendUpdate();
            scheduleAt(simTime() + updateInterval, timer);
        }
        else if (msg == flTimer) {
            aggregateFL();          // average received models and replace local
            sendFLModel();          // broadcast current weights
            scheduleAt(simTime() + flInterval, flTimer);
        }
    }
    else if (auto *hello = dynamic_cast<QRoutingHello *>(msg)) {
        processHello(hello, msg->getArrivalGate()->getIndex());
        delete msg;
    }
    else if (auto *update = dynamic_cast<QRoutingUpdate *>(msg)) {
        processUpdate(update, msg->getArrivalGate()->getIndex());
        delete msg;
    }
    else if (auto *flMsg = dynamic_cast<FLModel *>(msg)) {
        processFLModel(flMsg);
        delete msg;
    }
    else if (auto *data = dynamic_cast<QData *>(msg)) {
        if (data->getDestId() == myId) {
            send(data, "toApp");    // deliver to local App
        } else {
            forwardData(data);
        }
    }
    else {
        delete msg;
    }
}

// ---------- Hello & neighbour discovery ----------
void MADRLRouting_FL::processHello(QRoutingHello *hello, int gateIndex)
{
    int nbId = hello->getSrcNodeId();
    NeighborInfo ni;
    ni.id = nbId;
    ni.x = hello->getX();
    ni.y = hello->getY();
    ni.gateIndex = gateIndex;
    // Fake additional features (could be read from real cross‑layer data)
    ni.battery = uniform(0.2, 1.0);
    ni.queueLen = uniform(0, 10);
    ni.rssi = uniform(-80, -40);
    neighbors[nbId] = ni;

    if (std::find(neighborList.begin(), neighborList.end(), nbId) == neighborList.end()) {
        if (neighborList.size() < MAX_NEIGHBORS)
            neighborList.push_back(nbId);
    }
    EV << "Node " << myId << " learned neighbour " << nbId << "\n";
}

// ---------- Q‑update reception (keep compatible with older nodes if needed) ----------
void MADRLRouting_FL::processUpdate(QRoutingUpdate *update, int gateIndex)
{
    int nbId = update->getSrcNodeId();
    // Ensure neighbour record exists
    if (neighbors.find(nbId) == neighbors.end()) {
        NeighborInfo ni;
        ni.id = nbId;
        ni.x = -1; ni.y = -1;
        ni.battery = 0.5; ni.queueLen = 0; ni.rssi = -60;
        ni.gateIndex = gateIndex;
        neighbors[nbId] = ni;
        if (neighborList.size() < MAX_NEIGHBORS)
            neighborList.push_back(nbId);
    }
    // Not used in GNN‑MADRL – the agent learns from its own experience, not from neighbour Q‑tables.
}

// ---------- Federated Learning ----------
void MADRLRouting_FL::sendFLModel()
{
    FLModel *flmsg = new FLModel("FLModel");
    flmsg->setSrcId(myId);
    flmsg->setDestId(-1);   // broadcast
    std::vector<double> flatWeights = qNetwork->getFlatWeights();
    flmsg->setWeightsArraySize(flatWeights.size());
    for (size_t i = 0; i < flatWeights.size(); i++)
        flmsg->setWeights(i, flatWeights[i]);

    for (int i = 0; i < gateSize("out"); i++) {
        FLModel *copy = flmsg->dup();
        send(copy, "out", i);
    }
    delete flmsg;
}

void MADRLRouting_FL::processFLModel(FLModel *msg)
{
    int srcId = msg->getSrcId();
    int n = msg->getWeightsArraySize();
    std::vector<double> weights(n);
    for (int i = 0; i < n; i++)
        weights[i] = msg->getWeights(i);
    neighborModels[srcId] = weights;
}

void MADRLRouting_FL::aggregateFL()
{
    if (neighborModels.empty()) return;

    // Average of all received models + own model
    size_t nParams = qNetwork->totalParams();
    std::vector<double> avgWeights(nParams, 0.0);

    // Own model
    std::vector<double> own = qNetwork->getFlatWeights();
    for (size_t i = 0; i < nParams; i++) avgWeights[i] += own[i];

    // Neighbour models
    for (auto &entry : neighborModels) {
        for (size_t i = 0; i < nParams; i++)
            avgWeights[i] += entry.second[i];
    }
    int count = 1 + (int)neighborModels.size();
    for (size_t i = 0; i < nParams; i++) avgWeights[i] /= count;

    qNetwork->setFlatWeights(avgWeights);
    targetNetwork->setFlatWeights(avgWeights);   // also update target for stability
    neighborModels.clear();
    EV << "Node " << myId << " FL aggregated " << count << " models.\n";
}

// ---------- Feature construction ----------
std::vector<double> MADRLRouting_FL::buildSelfFeatures()
{
    std::vector<double> feats(FEAT_DIM);
    cModule *parent = getParentModule();
    feats[0] = atof(parent->getDisplayString().getTagArg("p", 0)) / 1500.0; // normalised x
    feats[1] = atof(parent->getDisplayString().getTagArg("p", 1)) / 1500.0; // normalised y
    feats[2] = 1.0;        // own battery (full)
    feats[3] = 0.0;        // own queue length
    feats[4] = 0.0;        // own RSSI (no meaning)
    return feats;
}

std::vector<std::vector<double>> MADRLRouting_FL::buildNeighborFeatures()
{
    std::vector<std::vector<double>> list;
    for (int nbId : neighborList) {
        auto it = neighbors.find(nbId);
        if (it != neighbors.end()) {
            std::vector<double> f(FEAT_DIM);
            f[0] = it->second.x / 1500.0;
            f[1] = it->second.y / 1500.0;
            f[2] = it->second.battery;
            f[3] = it->second.queueLen / 10.0; // normalised
            f[4] = (it->second.rssi + 100) / 60.0; // map -100..-40 → 0..1
            list.push_back(f);
        }
    }
    return list;
}

// ---------- Action selection ----------
int MADRLRouting_FL::selectAction(const std::vector<double>& selfFeat,
                                  const std::vector<std::vector<double>>& neighFeats)
{
    if (neighborList.empty()) return -1;

    // ε‑greedy
    if (uniform(0,1) < epsilon) {
        return intrand(neighborList.size());
    }

    std::vector<double> qValues = qNetwork->forward(selfFeat, neighFeats);
    // Only consider valid neighbours (up to actual count)
    double bestQ = -1e9;
    int bestIdx = 0;
    for (int i = 0; i < (int)neighborList.size(); i++) {
        if (qValues[i] > bestQ) {
            bestQ = qValues[i];
            bestIdx = i;
        }
    }
    return bestIdx;
}

// ---------- Forwarding ----------
void MADRLRouting_FL::forwardData(QData *data)
{
    int dest = data->getDestId();
    if (data->getTtl() <= 0) {
        EV << "TTL expired, dropping\n";
        delete data;
        return;
    }

    std::vector<double> selfFeat = buildSelfFeatures();
    std::vector<std::vector<double>> neighFeats = buildNeighborFeatures();
    int actionIdx = selectAction(selfFeat, neighFeats);

    if (actionIdx < 0 || actionIdx >= (int)neighborList.size()) {
        EV << "No valid action, dropping packet\n";
        delete data;
        return;
    }

    int nextHop = neighborList[actionIdx];
    int gateIdx = neighbors[nextHop].gateIndex;

    // Compute immediate reward (negative distance, to favour short paths)
    double dx = atof(getParentModule()->getDisplayString().getTagArg("p", 0)) - neighbors[nextHop].x;
    double dy = atof(getParentModule()->getDisplayString().getTagArg("p", 1)) - neighbors[nextHop].y;
    double dist = sqrt(dx*dx + dy*dy) / 1500.0;   // normalised
    double reward = -dist;

    // Build next state (after forwarding – we approximate by using current state again; ideal would be actual next hop's state)
    std::vector<double> nextSelf = buildSelfFeatures();   // same node
    std::vector<std::vector<double>> nextNeigh = neighFeats; // same neighbours

    // Store experience
    Experience exp;
    exp.state = selfFeat;
    exp.neighStates = neighFeats;
    exp.action = actionIdx;
    exp.reward = reward;
    exp.nextSelfState = nextSelf;
    exp.nextNeighStates = nextNeigh;
    exp.done = (nextHop == dest);   // true if next hop is the destination
    storeExperience(exp);

    // Update packet and forward
    data->setHopCount(data->getHopCount() + 1);
    data->setTtl(data->getTtl() - 1);
    simtime_t linkDelay = 0.001;   // 1 ms link delay
    sendDelayed(data, linkDelay, "out", gateIdx);
}

// ---------- Experience replay ----------
void MADRLRouting_FL::storeExperience(const Experience& exp)
{
    if (replayBuffer.size() >= (size_t)replayCapacity)
        replayBuffer.pop_front();
    replayBuffer.push_back(exp);
}

void MADRLRouting_FL::train()
{
    if (replayBuffer.size() < (size_t)batchSize) return;

    std::vector<int> indices(replayBuffer.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::random_shuffle(indices.begin(), indices.end());

    for (int i = 0; i < batchSize; i++) {
        Experience &exp = replayBuffer[indices[i]];
        // Online Q‑values
        std::vector<double> qOnline = qNetwork->forward(exp.state, exp.neighStates);
        // Target Q‑values
        std::vector<double> qTargetVal = targetNetwork->forward(exp.nextSelfState, exp.nextNeighStates);
        double maxNextQ = -1e9;
        for (double v : qTargetVal) if (v > maxNextQ) maxNextQ = v;
        double target = exp.reward + (exp.done ? 0 : discountFactor * maxNextQ);
        // Create target vector for backprop (simple approach: adjust only chosen action)
        std::vector<double> targetVec = qOnline;
        if (exp.action >= 0 && exp.action < (int)targetVec.size())
            targetVec[exp.action] = target;

        // We'll perform one step of gradient descent using a very rudimentary method.
        // Because our NeuralNet already provides backward(), we can use it.
        // However, backward expects input and target for the *whole* network, not just the action.
        // For simplicity, we'll call the MLP part's backward only (the GNN layers do not have a backward implemented).
        // In practice, you would implement a full backprop for the GNN. Here we skip it and only train the MLP head.
        // For a complete implementation, replace this with a proper loss function.
        // This is a placeholder: we call the MLP backward with the same input but target modified.
        // It will not alter the GNN weights, but it's a start.
        qNetwork->mlp.backward(exp.state, targetVec);   // only trains the final MLP
    }

    trainStepCount++;
    if (trainStepCount % TARGET_UPDATE_FREQ == 0) {
        targetNetwork->setFlatWeights(qNetwork->getFlatWeights());
    }
}

// ---------- Periodic update (broadcast own Q‑table summary – kept minimal) ----------
void MADRLRouting_FL::sendUpdate()
{
    // For compatibility, we still send a QRoutingUpdate with dummy data.
    // The FL messages carry the actual model parameters.
    QRoutingUpdate *update = new QRoutingUpdate("QRoutingUpdate");
    update->setSrcNodeId(myId);
    update->setDestIdArraySize(1);
    update->setMinQArraySize(1);
    update->setDestId(0, myId);
    update->setMinQ(0, 0.0);

    for (int i = 0; i < gateSize("out"); i++) {
        QRoutingUpdate *copy = update->dup();
        send(copy, "out", i);
    }
    delete update;

    // Perform training on recent experiences
    train();
}

void MADRLRouting_FL::finish()
{
    cancelAndDelete(timer);
    cancelAndDelete(flTimer);
}
