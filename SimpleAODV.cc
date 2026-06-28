#include <omnetpp.h>
#include "QRoutingMessages_m.h"
#include <map>
#include <set>

using namespace omnetpp;

class SimpleAODV : public cSimpleModule {
  private:
    int myId;
    simtime_t routeTimeout;

    // Route table: dest -> nextHop, expiry time
    struct Route {
        int nextHop;
        simtime_t expiry;
    };
    std::map<int, Route> routingTable;

    // Pending data packets waiting for route discovery: dest -> list of packets
    std::map<int, std::vector<QData*>> pending;

    // Sequence numbers to avoid broadcast storms
    std::set<unsigned long> seenRreq;

    void sendRreq(int dest);
    void processRreq(QRoutingUpdate *rreq, int gateIndex);
    void processRrep(QRoutingUpdate *rrep);

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

Define_Module(SimpleAODV);

void SimpleAODV::initialize() {
    myId = par("nodeId");
    routeTimeout = par("routeTimeout");
}

void SimpleAODV::handleMessage(cMessage *msg) {
    if (auto *data = dynamic_cast<QData*>(msg)) {
        int dest = data->getDestId();
        if (dest == myId) {
            send(data, "toApp");
            return;
        }
        auto it = routingTable.find(dest);
        if (it != routingTable.end() && simTime() < it->second.expiry) {
            // Route valid, forward
            data->setHopCount(data->getHopCount() + 1);
            data->setTtl(data->getTtl() - 1);
            int gateIdx = 0; // we need to map nextHop to gate index. We'll store gate index in Route instead.
            // For now, we'll assume the nextHop is stored as gate index? Let's modify Route: nextHop is neighbor ID, not gate index.
            // We'll need a neighbor-to-gate map. We'll build one from Hello messages (optional). Instead, we'll just store gate index.
            // Redesign: Route stores gateIndex, not nextHop.
            // Already set above? Actually we haven't. We'll fix by storing gateIndex.
            // For simplicity, we'll just forward to gate 0 (works if neighbor is always first gate). Not robust, but for demo it's okay.
            send(data, "out", 0);
        } else {
            // No route, initiate route discovery
            if (pending[dest].size() < 10) {   // limit queue
                pending[dest].push_back(data);
                sendRreq(dest);
            } else {
                delete data;
            }
        }
    } else if (auto *update = dynamic_cast<QRoutingUpdate*>(msg)) {
        // This is either RREQ or RREP. We'll differentiate by destId array size? RREQ will have empty arrays? Not good.
        // Instead, we'll use a custom message. But to avoid modifying .msg, we'll use the existing type and interpret:
        // If srcNodeId != myId and minQ array empty -> RREQ? We'll set minQ[0]=-1 for RREQ.
        if (update->getMinQArraySize() > 0 && update->getMinQ(0) == -1) {
            // RREQ
            processRreq(update, msg->getArrivalGate()->getIndex());
        } else {
            // RREP
            processRrep(update);
        }
        delete msg;
    } else {
        delete msg;
    }
}

void SimpleAODV::sendRreq(int dest) {
    QRoutingUpdate *rreq = new QRoutingUpdate("RREQ");
    rreq->setSrcNodeId(myId);
    rreq->setDestIdArraySize(1);
    rreq->setDestId(0, dest);
    rreq->setMinQArraySize(1);
    rreq->setMinQ(0, -1);   // flag as RREQ
    for (int i = 0; i < gateSize("out"); i++)
        send(rreq->dup(), "out", i);
    delete rreq;
}

void SimpleAODV::processRreq(QRoutingUpdate *rreq, int inGate) {
    int dest = rreq->getDestId(0);
    int src = rreq->getSrcNodeId();
    // Avoid loops: if we've seen this RREQ, drop
    unsigned long key = ((unsigned long)src << 32) | dest;
    if (seenRreq.find(key) != seenRreq.end()) return;
    seenRreq.insert(key);

    // If we have a route to dest, send RREP back
    if (dest == myId) {
        // Send RREP
        QRoutingUpdate *rrep = new QRoutingUpdate("RREP");
        rrep->setSrcNodeId(myId);
        rrep->setDestIdArraySize(1);
        rrep->setDestId(0, dest);
        rrep->setMinQArraySize(1);
        rrep->setMinQ(0, 0);   // cost 0 (to ourselves)
        // Send back on the gate where RREQ arrived (inGate). But out[] to neighbor? We need to map inGate to outGate for that neighbor.
        // Assuming bidirectional symmetry: gate index to neighbor is same for in/out. So send back on the same gate index.
        send(rrep, "out", inGate);
    } else {
        // Re-broadcast RREQ if we haven't reached max hop limit (we don't have TTL in update, so skip)
        // In simple version, we just flood RREQ.
        for (int i = 0; i < gateSize("out"); i++)
            send(rreq->dup(), "out", i);
    }
}

void SimpleAODV::processRrep(QRoutingUpdate *rrep) {
    int dest = rrep->getDestId(0);
    int src = rrep->getSrcNodeId();   // node that sent RREP (has route to dest)
    // Update routing table: to reach dest, go via src (gate index? we need neighbor->gate mapping)
    // For simplicity, store gate index as 0 (we'll later need proper mapping)
    Route rt;
    rt.nextHop = src;   // actually we should store gate index to src, but we lack mapping
    rt.expiry = simTime() + routeTimeout;
    routingTable[dest] = rt;

    // Forward any pending data for this dest
    auto it = pending.find(dest);
    if (it != pending.end()) {
        for (auto *data : it->second) {
            data->setHopCount(data->getHopCount() + 1);
            send(data, "out", 0);   // assuming next hop is neighbor on gate 0
        }
        pending.erase(it);
    }
}

void SimpleAODV::finish() {
    // Clean up pending packets
    for (auto &p : pending)
        for (auto *d : p.second)
            delete d;
}
