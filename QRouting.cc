#include "QRouting.h"
#include <climits>
#include <cmath>
#include <cstdlib>
Define_Module(QRouting);

void QRouting::initialize()
{
    myId = par("nodeId");
    alpha = par("alpha");
    updateInterval = par("updateInterval");
    defaultCost = par("costMetric");

    // read own position from parent module's display string
    cModule *parent = getParentModule();
    double myX = atof(parent->getDisplayString().getTagArg("p", 0));
    double myY = atof(parent->getDisplayString().getTagArg("p", 1));

    // own destination Q = 0
    qTable[myId][myId] = 0.0;

    // send hello with position
    for (int i = 0; i < gateSize("out"); i++) {
        QRoutingHello *hello = new QRoutingHello("hello");
        hello->setSrcNodeId(myId);
        hello->setX(myX);
        hello->setY(myY);
        send(hello, "out", i);
    }

    timer = new cMessage("updateTimer");
    scheduleAt(simTime() + uniform(0, updateInterval), timer);
    WATCH(myId);
}

void QRouting::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        sendUpdate();
        scheduleAt(simTime() + updateInterval, timer);
    }
    else if (auto *hello = dynamic_cast<QRoutingHello *>(msg)) {
        int gateIndex = msg->getArrivalGate()->getIndex();
        processHello(hello, gateIndex);
        delete msg;
    }
    else if (auto *update = dynamic_cast<QRoutingUpdate *>(msg)) {
        int gateIndex = msg->getArrivalGate()->getIndex();
        processUpdate(update, gateIndex);
        delete msg;
    }
    else if (auto *data = dynamic_cast<QData *>(msg)) {
        if (data->getDestId() == myId) {
            // deliver to app
            send(data, "toApp");
        } else {
            forwardData(data);
        }
    }
    else {
        delete msg;
    }
}

void QRouting::processHello(QRoutingHello *hello, int gateIndex)
{
    int nbId = hello->getSrcNodeId();
    double nx = hello->getX();
    double ny = hello->getY();

    // store neighbor info
    NeighborInfo ni;
    ni.id = nbId;
    ni.x = nx;
    ni.y = ny;
    ni.gateIndex = gateIndex;
    neighbors[nbId] = ni;

    // Update Q-value for this neighbor with distance-based cost
    double dist = distanceToNeighbor(nbId);
    double cost = (dist <= 0) ? 1.0 : dist / 100.0;  // scale: 100m -> cost 1

    // For all known destinations, initialise Q to this neighbor if not set
    for (auto &destEntry : qTable) {
        int dest = destEntry.first;
        if (qTable[dest].find(nbId) == qTable[dest].end()) {
            qTable[dest][nbId] = cost;
        }
    }
    // also ensure self destination has this neighbor entry with cost
    qTable[myId][nbId] = cost;

    EV << "Node " << myId << " learned neighbor " << nbId
       << " at (" << nx << "," << ny << ") distance=" << dist << " cost=" << cost << endl;
}

void QRouting::processUpdate(QRoutingUpdate *update, int gateIndex)
{
    int nbId = update->getSrcNodeId();

    // ensure neighbor info exists (should from hello)
    if (neighbors.find(nbId) == neighbors.end()) {
        // assume position unknown, use defaultCost
        NeighborInfo ni;
        ni.id = nbId;
        ni.x = -1;
        ni.y = -1;
        ni.gateIndex = gateIndex;
        neighbors[nbId] = ni;
    }

    double cost = distanceToNeighbor(nbId);
    if (cost <= 0) cost = defaultCost;   // fallback

    auto &myTable = qTable;

    int n = update->getDestIdArraySize();
    for (int i = 0; i < n; i++) {
        int dest = update->getDestId(i);
        double nbMinQ = update->getMinQ(i);

        double oldQ = myTable[dest][nbId];
        double newQ = oldQ + alpha * (nbMinQ + cost - oldQ);

        if (newQ < 0) newQ = 0;
        if (newQ > 1e9) newQ = 1e9;

        myTable[dest][nbId] = newQ;

        EV << "Node " << myId << " update Q[" << dest << "][" << nbId << "] = "
           << oldQ << " -> " << newQ << " (neigh minQ=" << nbMinQ << ", cost=" << cost << ")" << endl;
    }
    updateDisplayString();
}

void QRouting::forwardData(QData *data)
{
    int dest = data->getDestId();
    int bestNb = getBestNeighbor(dest);
    if (bestNb == -1) {
        EV << "No route to " << dest << ", dropping packet\n";
        delete data;
        return;
    }
    // find gate index to that neighbor
    int gateIdx = neighbors[bestNb].gateIndex;
    send(data, "out", gateIdx);
}

double QRouting::getMinQ(int dest)
{
    if (dest == myId) return 0.0;
    auto it = qTable.find(dest);
    if (it == qTable.end()) return 9999.0;
    double best = 9999.0;
    for (auto &entry : it->second) {
        if (entry.second < best) best = entry.second;
    }
    return best;
}

int QRouting::getBestNeighbor(int dest)
{
    auto it = qTable.find(dest);
    if (it == qTable.end()) return -1;
    double best = 9999.0;
    int bestNb = -1;
    for (auto &entry : it->second) {
        if (entry.second < best) {
            best = entry.second;
            bestNb = entry.first;
        }
    }
    if (best >= 9999.0) return -1;
    return bestNb;
}

double QRouting::distanceToNeighbor(int neighborId)
{
    auto it = neighbors.find(neighborId);
    if (it == neighbors.end()) return -1;
    double myX = atof(getParentModule()->getDisplayString().getTagArg("p", 0));
    double myY = atof(getParentModule()->getDisplayString().getTagArg("p", 1));
    double dx = myX - it->second.x;
    double dy = myY - it->second.y;
    return sqrt(dx*dx + dy*dy);
}

void QRouting::sendUpdate()
{
    QRoutingUpdate *update = new QRoutingUpdate("QRoutingUpdate");
    update->setSrcNodeId(myId);

    std::vector<int> dests;
    std::vector<double> minQs;
    for (auto &destEntry : qTable) {
        int dest = destEntry.first;
        double mq = getMinQ(dest);
        if (mq < 9999.0) {
            dests.push_back(dest);
            minQs.push_back(mq);
        }
    }
    if (getMinQ(myId) >= 9999.0) {
        dests.push_back(myId);
        minQs.push_back(0.0);
    }

    unsigned int num = dests.size();
    update->setDestIdArraySize(num);
    update->setMinQArraySize(num);
    for (unsigned int i = 0; i < num; i++) {
        update->setDestId(i, dests[i]);
        update->setMinQ(i, minQs[i]);
    }

    for (int i = 0; i < gateSize("out"); i++) {
        QRoutingUpdate *copy = update->dup();
        send(copy, "out", i);
    }
    delete update;
    updateDisplayString();
}

void QRouting::updateDisplayString()
{
    char buf[64];
    snprintf(buf, sizeof(buf), "Node %d\n%d dests", myId, (int)qTable.size());
    getDisplayString().setTagArg("t", 0, buf);
}

void QRouting::finish()
{
    EV << "Final Q-table for node " << myId << ":\n";
    for (auto &destEntry : qTable) {
        int dest = destEntry.first;
        EV << "  dest " << dest << ": ";
        for (auto &nbEntry : destEntry.second) {
            EV << "via " << nbEntry.first << " = " << nbEntry.second << "  ";
        }
        EV << " (min=" << getMinQ(dest) << ")\n";
    }
    cancelAndDelete(timer);
}
