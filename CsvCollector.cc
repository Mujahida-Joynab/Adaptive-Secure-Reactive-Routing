#include <omnetpp.h>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include "QRoutingMessages_m.h"

using namespace omnetpp;

class CsvCollector : public cSimpleModule, public cListener {
  private:
    std::string filename;
    std::vector<std::string> records;

    void subscribeRecursive(cModule *parent, const char *classname) {
        for (cModule::SubmoduleIterator it(parent); !it.end(); ++it) {
            cModule *child = *it;
            if (strcmp(child->getClassName(), classname) == 0) {
                child->subscribe("packetReceived", this);
                std::cout << "Subscribed to " << child->getFullPath() << endl;
            }
            subscribeRecursive(child, classname);
        }
    }

  public:
    virtual void initialize() override;
    virtual void receiveSignal(cComponent *source, simsignal_t, cObject *obj, cObject *) override;
    virtual void finish() override;
};

Define_Module(CsvCollector);

void CsvCollector::initialize() {
    subscribeRecursive(getSystemModule(), "App");
    filename = par("csvFile").stringValue();
}

void CsvCollector::receiveSignal(cComponent *, simsignal_t, cObject *obj, cObject *) {
    QData *data = dynamic_cast<QData*>(obj);
    if (!data) return;
    char buf[256];
    snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%.6f",
             data->getSrcId(),
             data->getDestId(),
             data->getSeqNum(),                     // <-- added
             data->getHopCount(),
             (simTime() - data->getTimestamp()).dbl());
    records.push_back(buf);
}

void CsvCollector::finish() {
    std::ofstream out(filename);
    out << "Src,Dest,Seq,HopCount,Delay(s)\n";
    for (auto &r : records) out << r << "\n";
    out.close();
    std::cout << "CSV written: " << filename << " (" << records.size() << " packets)\n";
}
