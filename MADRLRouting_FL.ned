simple MADRLRouting_FL
{
    parameters:
        int nodeId;
        double alpha = default(0.001);
        double updateInterval @unit(s) = default(1s);
        double discountFactor = default(0.9);
        int replayCapacity = default(100);
        int batchSize = default(16);
        int hiddenDim = default(16);
        double epsilon = default(0.3);
        double flInterval @unit(s) = default(10s);
        @display("i=block/routing");

    gates:
        input in[];
        output out[];
        input fromApp;
        output toApp;
}
