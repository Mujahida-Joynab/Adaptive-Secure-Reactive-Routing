module NodeFL
{
    parameters:
        int nodeId;
        double alpha = default(0.001);
        double updateInterval @unit(s) = default(1s);
        double costMetric = default(1.0);
        double trafficRate @unit(s) = default(exponential(1s));
        double flInterval @unit(s) = default(10s);
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
        routing: MADRLRouting_FL {
            parameters:
                nodeId = parent.nodeId;
                alpha = parent.alpha;
                updateInterval = parent.updateInterval;
                discountFactor = 0.9;
                replayCapacity = 100;
                batchSize = 16;
                hiddenDim = 16;
                epsilon = 0.3;
                flInterval = parent.flInterval;
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