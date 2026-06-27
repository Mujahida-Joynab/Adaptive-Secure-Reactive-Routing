import Node;

network QNet
{
    parameters:
        int numNodes = default(10);
    submodules:
        node[numNodes]: Node {
            nodeId = index;
        }

    connections:
        // chain
        node[0].out++ --> node[1].in++;
        node[0].in++ <-- node[1].out++;
        node[1].out++ --> node[2].in++;
        node[1].in++ <-- node[2].out++;
        node[2].out++ --> node[3].in++;
        node[2].in++ <-- node[3].out++;
        node[3].out++ --> node[4].in++;
        node[3].in++ <-- node[4].out++;
        node[4].out++ --> node[5].in++;
        node[4].in++ <-- node[5].out++;
        node[5].out++ --> node[6].in++;
        node[5].in++ <-- node[6].out++;
        node[6].out++ --> node[7].in++;
        node[6].in++ <-- node[7].out++;
        node[7].out++ --> node[8].in++;
        node[7].in++ <-- node[8].out++;
        node[8].out++ --> node[9].in++;
        node[8].in++ <-- node[9].out++;

        // cross links
        node[0].out++ --> node[2].in++;
        node[0].in++ <-- node[2].out++;
        node[2].out++ --> node[5].in++;
        node[2].in++ <-- node[5].out++;
        node[5].out++ --> node[8].in++;
        node[5].in++ <-- node[8].out++;
}
