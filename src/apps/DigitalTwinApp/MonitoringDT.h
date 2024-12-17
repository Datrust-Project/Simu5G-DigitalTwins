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

#ifndef _CBRRECEIVER_H_
#define _CBRRECEIVER_H_

#include <string.h>
#include <omnetpp.h>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "MonitoringDTPacket_m.h"
#include "apps/cbr/CbrPacket_m.h"

namespace simu5g {

class MonitoringDT : public omnetpp::cSimpleModule
{
    inet::UdpSocket socket;

    int numReceived_;
    int totFrames_;
    int recvBytes_;

    bool mInit_;

    inet::simtime_t lastPayloadTimestamp_;
    bool isFirstSample_;
    int numSamples_;

    static omnetpp::simsignal_t monitoringDTFrameLossSignal_;
    static omnetpp::simsignal_t monitoringDTFrameDelaySignal_;
    static omnetpp::simsignal_t monitoringDTJitterSignal_;
    static omnetpp::simsignal_t monitoringDTReceivedThroughtput_;
    static omnetpp::simsignal_t monitoringDTReceivedBytesSignal_;

    omnetpp::simsignal_t monitoringDTRcvdPkt_;

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override;
    virtual void finish() override;
  public:
    inet::simtime_t getLastPayloadTimestamp() { return lastPayloadTimestamp_; }
    int getNumSamples() { return numSamples_; }
    void flushSamples();
};

} //namespace

#endif

