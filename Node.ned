module Node
{
    parameters:
        int nodeId;
        double alpha = default(0.7);
        double updateInterval @unit(s) = default(1s);
        double costMetric = default(1.0);
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
        routing: QRouting {
            parameters:
                nodeId = parent.nodeId;
                alpha = parent.alpha;
                updateInterval = parent.updateInterval;
                costMetric = parent.costMetric;
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
