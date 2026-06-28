import Node;
import NodeFlood;
import NodeAODV;
import NodeDV;
import NodeIdeal;
import NodeFL;
import NodeMADQN;
import CsvCollector;

network QNet
{
    parameters:
        int numNodes = default(10);
    submodules:
        collector: CsvCollector {
            csvFile = "results-${configname}-${repetition}.csv";
        }
        node[numNodes]: Node {
            nodeId = index;
        }
    connections:
        for i=0..numNodes-1 {
            node[i].out++ --> node[(i+1) % numNodes].in++;
            node[i].in++ <-- node[(i+1) % numNodes].out++;
            node[i].out++ --> node[(i-1+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-1+numNodes) % numNodes].out++;
            node[i].out++ --> node[(i+3) % numNodes].in++;
            node[i].in++ <-- node[(i+3) % numNodes].out++;
            node[i].out++ --> node[(i-3+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-3+numNodes) % numNodes].out++;
        }
}

network QNet_Flood
{
    parameters:
        int numNodes = default(10);
    submodules:
        collector: CsvCollector {
            csvFile = "results-${configname}-${repetition}.csv";
        }
        node[numNodes]: NodeFlood {
            nodeId = index;
        }
    connections:
        for i=0..numNodes-1 {
            node[i].out++ --> node[(i+1) % numNodes].in++;
            node[i].in++ <-- node[(i+1) % numNodes].out++;
            node[i].out++ --> node[(i-1+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-1+numNodes) % numNodes].out++;
            node[i].out++ --> node[(i+3) % numNodes].in++;
            node[i].in++ <-- node[(i+3) % numNodes].out++;
            node[i].out++ --> node[(i-3+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-3+numNodes) % numNodes].out++;
        }
}

network QNet_AODV
{
    parameters:
        int numNodes = default(10);
    submodules:
        collector: CsvCollector {
            csvFile = "results-${configname}-${repetition}.csv";
        }
        node[numNodes]: NodeAODV {
            nodeId = index;
        }
    connections:
        for i=0..numNodes-1 {
            node[i].out++ --> node[(i+1) % numNodes].in++;
            node[i].in++ <-- node[(i+1) % numNodes].out++;
            node[i].out++ --> node[(i-1+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-1+numNodes) % numNodes].out++;
            node[i].out++ --> node[(i+3) % numNodes].in++;
            node[i].in++ <-- node[(i+3) % numNodes].out++;
            node[i].out++ --> node[(i-3+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-3+numNodes) % numNodes].out++;
        }
}

network QNet_DV
{
    parameters:
        int numNodes = default(10);
    submodules:
        collector: CsvCollector {
            csvFile = "results-${configname}-${repetition}.csv";
        }
        node[numNodes]: NodeDV {
            nodeId = index;
        }
    connections:
        for i=0..numNodes-1 {
            node[i].out++ --> node[(i+1) % numNodes].in++;
            node[i].in++ <-- node[(i+1) % numNodes].out++;
            node[i].out++ --> node[(i-1+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-1+numNodes) % numNodes].out++;
            node[i].out++ --> node[(i+3) % numNodes].in++;
            node[i].in++ <-- node[(i+3) % numNodes].out++;
            node[i].out++ --> node[(i-3+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-3+numNodes) % numNodes].out++;
        }
}

network QNet_Ideal
{
    parameters:
        int numNodes = default(10);
    submodules:
        collector: CsvCollector {
            csvFile = "results-${configname}-${repetition}.csv";
        }
        node[numNodes]: NodeIdeal {
            nodeId = index;
        }
    connections:
        for i=0..numNodes-1 {
            node[i].out++ --> node[(i+1) % numNodes].in++;
            node[i].in++ <-- node[(i+1) % numNodes].out++;
            node[i].out++ --> node[(i-1+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-1+numNodes) % numNodes].out++;
            node[i].out++ --> node[(i+3) % numNodes].in++;
            node[i].in++ <-- node[(i+3) % numNodes].out++;
            node[i].out++ --> node[(i-3+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-3+numNodes) % numNodes].out++;
        }
}

network QNet_FL
{
    parameters:
        int numNodes = default(10);
    submodules:
        collector: CsvCollector {
            csvFile = "results-${configname}-${repetition}.csv";
        }
        node[numNodes]: NodeFL {
            nodeId = index;
        }
    connections:
        for i=0..numNodes-1 {
            node[i].out++ --> node[(i+1) % numNodes].in++;
            node[i].in++ <-- node[(i+1) % numNodes].out++;
            node[i].out++ --> node[(i-1+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-1+numNodes) % numNodes].out++;
            node[i].out++ --> node[(i+3) % numNodes].in++;
            node[i].in++ <-- node[(i+3) % numNodes].out++;
            node[i].out++ --> node[(i-3+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-3+numNodes) % numNodes].out++;
        }
}

network QNet_MADQN
{
    parameters:
        int numNodes = default(10);
    submodules:
        collector: CsvCollector {
            csvFile = "results-${configname}-${repetition}.csv";
        }
        node[numNodes]: NodeMADQN {
            nodeId = index;
        }
    connections:
        for i=0..numNodes-1 {
            node[i].out++ --> node[(i+1) % numNodes].in++;
            node[i].in++ <-- node[(i+1) % numNodes].out++;
            node[i].out++ --> node[(i-1+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-1+numNodes) % numNodes].out++;
            node[i].out++ --> node[(i+3) % numNodes].in++;
            node[i].in++ <-- node[(i+3) % numNodes].out++;
            node[i].out++ --> node[(i-3+numNodes) % numNodes].in++;
            node[i].in++ <-- node[(i-3+numNodes) % numNodes].out++;
        }
}
