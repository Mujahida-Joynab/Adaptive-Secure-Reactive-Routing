#ifndef __QROUTING_H_
#define __QROUTING_H_

#include <omnetpp.h>
#include <map>
#include <vector>
#include "QRoutingMessages_m.h"

using namespace omnetpp;

class QRouting : public cSimpleModule
{
  protected:
    int myId;
    double alpha;
    double updateInterval;
    double defaultCost;   // kept as parameter but overridden by distance

    struct NeighborInfo {
        int id;
        double x, y;
        int gateIndex;
    };
    std::map<int, NeighborInfo> neighbors;   // neighborId -> info

    // Q-table: destId -> (neighbourId -> Q-value)
    std::map<int, std::map<int, double>> qTable;

    cMessage *timer;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    void sendUpdate();
    void processHello(QRoutingHello *hello, int gateIndex);
    void processUpdate(QRoutingUpdate *update, int gateIndex);
    void forwardData(QData *data);
    double getMinQ(int dest);
    int getBestNeighbor(int dest);
    double distanceToNeighbor(int neighborId);
    void updateDisplayString();
};

#endif
