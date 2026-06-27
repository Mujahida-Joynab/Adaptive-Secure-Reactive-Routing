#include <omnetpp.h>
#include <cstdlib>
#include <string>
#include "QRoutingMessages_m.h"

using namespace omnetpp;

class Mobility : public cSimpleModule
{
  private:
    double maxX, maxY, speed;
    double targetX, targetY;
    cMessage *moveTimer;

  public:
    Mobility() {}
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(Mobility);

void Mobility::initialize()
{
    maxX = par("maxX");
    maxY = par("maxY");
    speed = par("speed");

    // initial random position
    double x = uniform(0, maxX);
    double y = uniform(0, maxY);
    getParentModule()->getDisplayString().setTagArg("p", 0, std::to_string(x).c_str());
    getParentModule()->getDisplayString().setTagArg("p", 1, std::to_string(y).c_str());

    targetX = uniform(0, maxX);
    targetY = uniform(0, maxY);
    moveTimer = new cMessage("move");
    scheduleAt(simTime() + 0.1, moveTimer);
}

void Mobility::handleMessage(cMessage *msg)
{
    if (msg == moveTimer) {
        double curX = atof(getParentModule()->getDisplayString().getTagArg("p", 0));
        double curY = atof(getParentModule()->getDisplayString().getTagArg("p", 1));

        double dx = targetX - curX;
        double dy = targetY - curY;
        double dist = sqrt(dx*dx + dy*dy);
        double step = speed * 0.1;   // 0.1 s interval

        if (dist < step) {
            // arrived at target, pick new target
            getParentModule()->getDisplayString().setTagArg("p", 0, std::to_string(targetX).c_str());
            getParentModule()->getDisplayString().setTagArg("p", 1, std::to_string(targetY).c_str());
            targetX = uniform(0, maxX);
            targetY = uniform(0, maxY);
        } else {
            double newX = curX + (dx/dist)*step;
            double newY = curY + (dy/dist)*step;
            getParentModule()->getDisplayString().setTagArg("p", 0, std::to_string(newX).c_str());
            getParentModule()->getDisplayString().setTagArg("p", 1, std::to_string(newY).c_str());
        }

        scheduleAt(simTime() + 0.1, moveTimer);
    }
}
