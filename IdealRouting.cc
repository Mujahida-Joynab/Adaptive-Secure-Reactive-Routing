#include <omnetpp.h>
#include "QRoutingMessages_m.h"
#include <map>
#include <climits>
using namespace omnetpp;

class IdealRouting : public cSimpleModule {
  private:
    int myId;
    std::map<int, int> nextHop;
    int numNodes;

    void computeRoutes() {
        int neighbours[4] = {(myId+1)%numNodes, (myId-1+numNodes)%numNodes, (myId+3)%numNodes, (myId-3+numNodes)%numNodes};
        for (int dest = 0; dest < numNodes; dest++) {
            if (dest == myId) continue;
            int bestDist = INT_MAX, bestGate = -1;
            for (int gate = 0; gate < 4; gate++) {
                int nbr = neighbours[gate];
                int dist = std::min((dest - nbr + numNodes) % numNodes, (nbr - dest + numNodes) % numNodes);
                if (dist < bestDist) { bestDist = dist; bestGate = gate; }
            }
            if (bestGate != -1) nextHop[dest] = bestGate;
        }
    }

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(IdealRouting);

void IdealRouting::initialize() {
    myId = par("nodeId");
    numNodes = getSystemModule()->par("numNodes");
    computeRoutes();
}

void IdealRouting::handleMessage(cMessage *msg) {
    if (auto *data = dynamic_cast<QData*>(msg)) {
        int dest = data->getDestId();
        if (dest == myId) { send(data, "toApp"); return; }
        auto it = nextHop.find(dest);
        if (it == nextHop.end()) { delete data; return; }
        data->setHopCount(data->getHopCount() + 1);
        sendDelayed(data, 0.01, "out", it->second);
    } else { delete msg; }
}
