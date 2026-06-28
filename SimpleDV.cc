#include <omnetpp.h>
#include "QRoutingMessages_m.h"
#include <map>
#include <algorithm>

using namespace omnetpp;

class SimpleDV : public cSimpleModule {
  private:
    int myId;
    std::map<int, double> dist;    // dest -> cost
    std::map<int, int> nextHop;    // dest -> neighbour (node ID)
    cMessage *timer;
    void sendUpdate();
    void processUpdate(QRoutingUpdate *update);
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

Define_Module(SimpleDV);

void SimpleDV::initialize() {
    myId = par("nodeId");
    dist[myId] = 0.0;
    sendUpdate();
    timer = new cMessage("dvTimer");
    scheduleAt(simTime() + 1.0, timer);
}

void SimpleDV::handleMessage(cMessage *msg) {
    if (msg == timer) {
        sendUpdate();
        // Jitter the next timer by ±0.1s to prevent synchronization
        scheduleAt(simTime() + 1.0 + uniform(-0.1, 0.1), timer);
    } else if (auto *update = dynamic_cast<QRoutingUpdate*>(msg)) {
        processUpdate(update);
        delete msg;
    } else if (auto *data = dynamic_cast<QData*>(msg)) {
        int dest = data->getDestId();
        if (dest == myId) {
            send(data, "toApp");
            return;
        }
        auto it = nextHop.find(dest);
        if (it == nextHop.end()) {
            delete data;   // no route
        } else {
            int nbId = it->second;
            // Determine gate index (hardcoded for ring+shortcut topology)
            int N = getSystemModule()->par("numNodes");
            int gateIdx = -1;
            if (nbId == (myId+1)%N) gateIdx = 0;
            else if (nbId == (myId-1+N)%N) gateIdx = 1;
            else if (nbId == (myId+3)%N) gateIdx = 2;
            else if (nbId == (myId-3+N)%N) gateIdx = 3;
            if (gateIdx != -1) {
                data->setHopCount(data->getHopCount() + 1);
                send(data, "out", gateIdx);
            } else {
                delete data;
            }
        }
    } else {
        delete msg;
    }
}

void SimpleDV::sendUpdate() {
    QRoutingUpdate *update = new QRoutingUpdate("DV");
    update->setSrcNodeId(myId);
    update->setDestIdArraySize(dist.size());
    update->setMinQArraySize(dist.size());
    int i = 0;
    for (auto &entry : dist) {
        update->setDestId(i, entry.first);
        update->setMinQ(i, entry.second);
        i++;
    }
    for (int j = 0; j < gateSize("out"); j++)
        send(update->dup(), "out", j);
    delete update;
}

void SimpleDV::processUpdate(QRoutingUpdate *update) {
    int src = update->getSrcNodeId();
    double costToSrc = 1.0;   // all links cost 1
    int n = update->getDestIdArraySize();
    for (int i = 0; i < n; i++) {
        int dest = update->getDestId(i);
        double newDist = update->getMinQ(i) + costToSrc;
        auto it = dist.find(dest);
        if (it == dist.end() || newDist < it->second) {
            dist[dest] = newDist;
            nextHop[dest] = src;
            // DO NOT send update  only the periodic timer sends updateshere 
        }
    }
}

void SimpleDV::finish() {
    cancelAndDelete(timer);
}
