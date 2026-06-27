module Node
{
    parameters:
        int nodeId;
        double alpha = default(0.7);
        double updateInterval @unit(s) = default(1s);
        double costMetric = default(1.0);
        double trafficRate @unit(s) = default(exponential(1s));
        @display("p=100,100;i=device/wifilaptop;bgb=1309,484");

    gates:
        input in[];
        output out[];

    submodules:
        app: App {
            parameters:
                nodeId = parent.nodeId;
                trafficRate = parent.trafficRate;
                @display("p=845,216");
        }
        routing: QRouting {
            parameters:
                nodeId = parent.nodeId;
                alpha = parent.alpha;
                updateInterval = parent.updateInterval;
                costMetric = parent.costMetric;
                @display("p=532,160");
        }
        mobility: Mobility {
            @display("p=1136,377");
        }

    connections:
        app.out --> routing.fromApp;
        routing.toApp --> app.in;
        for i=0..sizeof(in)-1 {
            in[i] --> routing.in++;
            routing.out++ --> out[i];
        }
}
