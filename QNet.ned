import NodeFL;
import CsvCollector;

network QNet
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
        for i=0..numNodes-2 {
            node[i].out++ --> node[i+1].in++;
            node[i].in++ <-- node[i+1].out++;
        }
}
