#include <omnetpp.h>
#include "QRoutingMessages_m.h"
#include <set>

using namespace omnetpp;

class Flooding : public cSimpleModule {
  private:
    int myId;
    std::set<unsigned long> seen;
    int seenCount = 0;
    static const int MAX_SEEN = 5000;   // limit memory usage
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(Flooding);

void Flooding::initialize() {
    myId = par("nodeId");
}

void Flooding::handleMessage(cMessage *msg) {
    if (auto *data = dynamic_cast<QData*>(msg)) {
        if (data->getDestId() == myId) {
            send(data, "toApp");
            return;
        }
        unsigned long key = ((unsigned long)data->getSrcId() << 32) | data->getSeqNum();
        if (seen.find(key) != seen.end()) {
            delete data;
            return;
        }
        // Limit memory: if set gets too large, clear it (may re‑flood some packets, but that's acceptable)
        if (seen.size() > MAX_SEEN) {
            seen.clear();
        }
        seen.insert(key);
        for (int i = 0; i < gateSize("out"); i++) {
            QData *copy = data->dup();
            copy->setHopCount(copy->getHopCount() + 1);
            copy->setTtl(copy->getTtl() - 1);
            if (copy->getTtl() > 0)
                send(copy, "out", i);
            else
                delete copy;
        }
        delete data;
    } else {
        delete msg;
    }
}
