packet QRoutingHello
{
    int srcNodeId;
    double x;
    double y;
}

packet QRoutingUpdate
{
    int srcNodeId;
    int destId[];
    double minQ[];
}

packet QData
{
    int srcId;
    int destId;
    int seqNum;
    int ttl = 32;            // plain initialiser
    int hopCount = 0;        // plain initialiser
    simtime_t timestamp;
}