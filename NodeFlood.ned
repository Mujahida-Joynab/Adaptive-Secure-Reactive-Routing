module NodeFlood
{
    parameters:
        int nodeId;
        double trafficRate @unit(s) = default(exponential(1s));
        @display("p=100,100;i=device/wifilaptop");
    gates:
        input in[];
        output out[];
    submodules:
        app: App {
            parameters:
                nodeId = parent.nodeId;
                trafficRate = parent.trafficRate;
            @display("p=100,50");
        }
        routing: Flooding {
            parameters:
                nodeId = parent.nodeId;
            @display("p=100,150");
        }
        mobility: Mobility {
            @display("p=200,150");
        }
    connections:
        app.out --> routing.fromApp;
        routing.toApp --> app.in;
        for i=0..sizeof(in)-1 {
            in[i] --> routing.in++;
            routing.out++ --> out[i];
        }
}
