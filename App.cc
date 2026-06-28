#include <omnetpp.h>
#include "QRoutingMessages_m.h"

using namespace omnetpp;

class App : public cSimpleModule
{
  private:
    int myId;
    int seqNum = 0;
    cMessage *timer;
    simsignal_t packetReceivedSignal;

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(App);

void App::initialize()
{
    myId = par("nodeId");
    timer = new cMessage("gen");
    scheduleAt(simTime() + par("trafficRate"), timer);
    packetReceivedSignal = registerSignal("packetReceived");
}

void App::handleMessage(cMessage *msg)
{
    if (msg == timer) {
        int numNodes = getSystemModule()->par("numNodes");
        int dest = intrand(numNodes);
        while (dest == myId)
            dest = intrand(numNodes);

        QData *data = new QData("Data");
        data->setSrcId(myId);
        data->setDestId(dest);
        data->setSeqNum(seqNum++);
        data->setTimestamp(simTime());
        send(data, "out");

        scheduleAt(simTime() + par("trafficRate"), timer);
    } else {
        QData *data = check_and_cast<QData*>(msg);
        EV << "App: received packet from " << data->getSrcId()
           << " seq=" << data->getSeqNum()
           << " delay=" << simTime() - data->getTimestamp() << endl;

        // Emit the signal so the CsvCollector records this delivery
        emit(packetReceivedSignal, data);

        delete msg;
    }
}
