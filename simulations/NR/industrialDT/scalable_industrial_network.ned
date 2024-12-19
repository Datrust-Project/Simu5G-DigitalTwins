//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.simulations.NR.industrialDT;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.ethernet.EthernetSwitch;
import inet.node.internetcloud.InternetCloud;
import ned.IdealChannel;
import simu5g.nodes.mec.ComputeNode.ComputeNodeMobile;
import simu5g.nodes.mec.ComputeNode.ComputeNodeFixed;
import simu5g.world.radio.LteChannelControl;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.Upf;
import simu5g.nodes.NR.gNodeB;
import simu5g.nodes.NR.NRUe;
import simu5g.nodes.mec.MECHost;

import inet.node.inet.Router;
import inet.node.inet.StandardHost;


network scalable_industrial_network
{
    parameters:
        @display("i=block/network2;bgb=1018.93396,615.24396;bgi=background/pisa");

        int numPAs = default(0);
        int numLocalDTs = default(0);
        int numFixedDTs = default(0);
        int numFixedPAs = default(0);        

        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)

    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=877.6425,122.64;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=968.34503,122.64;is=s");
        }
        channelControl: LteChannelControl {
            @display("p=876.365,24.272501;is=s");
        }
        binder: Binder {
            @display("p=976.01,200.5675;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=877.6425,209.51;is=s");
        }
        upf: Upf {
            @display("p=537.8275,249.1125");
        }
        iUpf: Upf {
            @display("p=352.92624,249.77875");
        }
        gNodeB1: gNodeB {
            @display("p=130.816,249.368;is=vl");
        }
        CloudServer: ComputeNodeFixed {
            @display("p=743.505,83.0375;is=n;i=device/router");
        }
        cloud_router: InternetCloud {
            @display("p=743.505,249.1125;i=misc/cloud");
        }

        PA[numPAs]: NRUe {
            @display("p=86.87,387.0825;i=block/cogwheel");
        }
        localDTnode[numLocalDTs]: ComputeNodeMobile {
            @display("p=196.735,387.0825;i=old/telnet");
        }
        EdgeServer: ComputeNodeFixed {
            @display("p=352.59,83.0375");
        }
        fixedPA[numFixedPAs]: StandardHost {
            @display("p=431.795,518.665");
        }
        fixedDT[numFixedDTs]: ComputeNodeFixed {
            @display("p=130.30501,513.555");
        }

        local_router: InternetCloud {
            @display("p=284.8825,513.555;i=misc/cloud");
        }
    //        edge_router: InternetCloud {
    //            @display("p=537.8275,68.985;i=misc/cloud");
    //        }
    connections allowunconnected:
        upf.pppg++ <--> Eth10G <--> iUpf.pppg++;
        iUpf.pppg++ <--> Eth10G <--> gNodeB1.ppp;
        CloudServer.pppg++ <--> Eth10G <--> cloud_router.pppg++;
        cloud_router.pppg++ <--> Eth10G <--> upf.filterGate;
        //        EdgeServer.pppg++ <--> Eth10G <--> edge_router.pppg++;
        //        iUpf.filterGate <--> Eth10G <--> edge_router.pppg++;
        EdgeServer.pppg++ <--> Eth10G <--> cloud_router.pppg++;
        
        for i=0..numFixedDTs-1 
        {
            fixedDT[i].ethg++ <--> Eth10G <--> local_router.ethg++;
        }
        
        for i=0..numFixedPAs-1 
        {
            fixedPA[i].ethg++ <--> Eth10G <--> local_router.ethg++;
        }
        
        local_router.ethg++ <--> Eth10G <--> cloud_router.ethg++;
}
