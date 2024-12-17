//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.apps.DigitalTwinApp;

import inet.applications.contract.IApp;


simple MonitoringDT like IApp
{
    parameters:
        int localPort = default(3000);

        @signal[monitoringDTFrameDelaySignal](type="simtime_t");
        @signal[monitoringDTJitterSignal](type="simtime_t");
        @signal[monitoringDTFrameLossSignal](type="double");
        @signal[monitoringDTReceivedThroughtputSignal];
        @signal[monitoringDTReceivedBytesSignal];
        @statistic[monitoringDTFrameDelay](title="Cbr Frame Delay"; unit="s"; source="monitoringDTFrameDelaySignal"; record=mean,vector);
        @statistic[monitoringDTJitter](title="Cbr Playout Loss"; unit="s"; source="monitoringDTJitterSignal"; record=mean,vector);
        @statistic[monitoringDTFrameLoss](title="Cbr Frame Loss"; unit="ratio"; source="monitoringDTFrameLossSignal"; record=mean);
        @statistic[monitoringDTReceivedThroughtput](title="Throughput received at the application level"; unit="Bps"; source="monitoringDTReceivedThroughtputSignal"; record=timeavg,mean,sum);
        @statistic[monitoringDTReceivedBytes](title="Bytes received at the application level"; unit="Bps"; source="monitoringDTReceivedBytesSignal"; record=timeavg,mean,vector,sum);



        @signal[monitoringDTRcvdPkt];
        @statistic[monitoringDTRcvdPkt](title="Received packet ID"; unit=""; source="monitoringDTRcvdPkt"; record=vector);

        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}

