simple MADQNRouting
{
    parameters:
        int nodeId;
        double alpha = default(0.001);
        double updateInterval @unit(s) = default(1s);
        double discountFactor = default(0.9);
        int replayCapacity = default(100);
        int batchSize = default(16);
        int hiddenSize = default(32);
        double epsilon = default(0.3);
        @display("i=block/routing");
    gates:
        input in[];
        output out[];
        input fromApp;
        output toApp;
}
