simple QRouting
{
    parameters:
        int nodeId;
        double alpha = default(0.7);
        double updateInterval @unit(s) = default(1s);
        double costMetric = default(1.0);
        @display("i=block/routing");
    gates:
        input in[];
        output out[];
        input fromApp;
        output toApp;
}
