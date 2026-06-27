simple App
{
    parameters:
        int nodeId;
        double trafficRate @unit(s) = default(exponential(1s));
        @display("i=block/app");
    gates:
        output out;
        input in;
}
