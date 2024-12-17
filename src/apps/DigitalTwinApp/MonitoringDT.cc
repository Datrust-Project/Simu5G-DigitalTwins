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

#include "MonitoringDT.h"

namespace simu5g {

Define_Module(MonitoringDT);
using namespace inet;

simsignal_t MonitoringDT::monitoringDTFrameLossSignal_ = registerSignal("monitoringDTFrameLossSignal");
simsignal_t MonitoringDT::monitoringDTFrameDelaySignal_ = registerSignal("monitoringDTFrameDelaySignal");
simsignal_t MonitoringDT::monitoringDTJitterSignal_ = registerSignal("monitoringDTJitterSignal");
simsignal_t MonitoringDT::monitoringDTReceivedThroughtput_ = registerSignal("monitoringDTReceivedThroughtputSignal");
simsignal_t MonitoringDT::monitoringDTReceivedBytesSignal_ = registerSignal("monitoringDTReceivedBytesSignal");

void MonitoringDT::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        mInit_ = true;
        numReceived_ = 0;
        recvBytes_ = 0;
        monitoringDTRcvdPkt_ = registerSignal("monitoringDTRcvdPkt");

        numSamples_ = 0;
        lastPayloadTimestamp_ = 0;
        isFirstSample_ = true;
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        int port = par("localPort");
        EV << "MonitoringDT::initialize - binding to port: local:" << port << endl;
        if (port != -1)
        {
            socket.setOutputGate(gate("socketOut"));
            socket.bind(port);
        }
    }
}

void MonitoringDT::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        return;

    Packet* pPacket = check_and_cast<Packet*>(msg);
    auto monitoringDTHeader = pPacket->popAtFront<CbrPacket>();

    numReceived_++;
    totFrames_ = monitoringDTHeader->getNframes(); // XXX this value can be written just once
    int pktSize = (int)monitoringDTHeader->getPayloadSize();

    // just to make sure we do not update recvBytes AND we avoid dividing by 0
    if( simTime() > getSimulation()->getWarmupPeriod() )
    {
        recvBytes_ += pktSize;
        emit( monitoringDTReceivedBytesSignal_ , pktSize );
    }
    if(isFirstSample_)
    {
        EV << "MonitoringDT::handleMessage - receiving first sample for a new sampling period";
        lastPayloadTimestamp_ = monitoringDTHeader->getPayloadTimestamp();
        isFirstSample_ = false;
    }
    numSamples_++;

    simtime_t delay = simTime()-monitoringDTHeader->getPayloadTimestamp();
    emit(monitoringDTFrameDelaySignal_,delay );

    EV << "MonitoringDT::handleMessage - Packet received: FRAME[" << monitoringDTHeader->getIDframe() << "/" << monitoringDTHeader->getNframes() << "] with delay["<< delay << "]" << endl;
    EV << "MonitoringDT::handleMessage - numSamples available " << numSamples_ << endl;
    emit(monitoringDTRcvdPkt_, (long)monitoringDTHeader->getIDframe());

    delete msg;
}

void MonitoringDT::finish()
{
    double lossRate = 0;
    if(totFrames_ > 0)
        lossRate = 1.0-(numReceived_/(totFrames_*1.0));

    emit(monitoringDTFrameLossSignal_,lossRate);

    simtime_t elapsedTime = simTime() - getSimulation()->getWarmupPeriod();
    emit( monitoringDTReceivedThroughtput_, recvBytes_ / elapsedTime.dbl() );
}

void MonitoringDT::flushSamples()
{
    EV << "MonitoringDT::flushSamples - flushing samples" << endl;
    isFirstSample_ = true;
    numSamples_ = 0;
}

} //namespace

