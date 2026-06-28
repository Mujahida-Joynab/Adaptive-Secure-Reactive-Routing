simple SimpleAODV
{
    parameters:
        int nodeId;
        double routeTimeout @unit(s) = default(5s);
        @display("i=block/routing");
    gates:
        input in[];
        output out[];
        input fromApp;
        output toApp;
}
