packet QRoutingHello
{
    int srcNodeId;
    double x;       // position for distance-based cost
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
    simtime_t timestamp;  // for delay measurement
}