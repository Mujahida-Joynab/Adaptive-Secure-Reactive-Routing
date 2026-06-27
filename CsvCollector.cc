#include <omnetpp.h>
#include <fstream>
#include <vector>
#include "QRoutingMessages_m.h"

using namespace omnetpp;

class CsvCollector : public cSimpleModule, public cListener
{
  private:
    std::string filename;
    struct Record {
        int src;
        int dest;
        int hopCount;
        simtime_t delay;
    };
    std::vector<Record> records;

  public:
    CsvCollector() {}
    virtual ~CsvCollector() {}

  protected:
    virtual void initialize() override;
    virtual void finish() override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;
};

Define_Module(CsvCollector);

void CsvCollector::initialize()
{
    // Subscribe to packetReceived signal from all App modules
    cModule *system = getSystemModule();
    for (cModule::SubmoduleIterator it(system); !it.end(); ++it) {
        cModule *mod = *it;
        if (strcmp(mod->getClassName(), "App") == 0) {
            mod->subscribe("packetReceived", this);
        }
    }
    filename = par("csvFile").stringValue();
    EV << "CsvCollector: writing to " << filename << endl;
}

void CsvCollector::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if (obj == nullptr) return;
    QData *data = dynamic_cast<QData*>(obj);
    if (!data) return;

    Record rec;
    rec.src = data->getSrcId();
    rec.dest = data->getDestId();
    rec.hopCount = data->getHopCount();
    rec.delay = simTime() - data->getTimestamp();
    records.push_back(rec);
}

void CsvCollector::finish()
{
    std::ofstream csvFile(filename);
    if (!csvFile.is_open()) {
        EV_ERROR << "Cannot open CSV file " << filename << endl;
        return;
    }
    csvFile << "Src,Dest,HopCount,Delay(s)\n";
    for (auto &r : records) {
        csvFile << r.src << "," << r.dest << "," << r.hopCount << "," << r.delay << "\n";
    }
    csvFile.close();
    EV << "CsvCollector: wrote " << records.size() << " entries to " << filename << endl;
}
