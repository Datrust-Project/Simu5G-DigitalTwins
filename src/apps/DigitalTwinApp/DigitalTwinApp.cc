//
//                  DT Simulator
//
// Authors: Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "DigitalTwinApp.h"

namespace simu5g {
Define_Module(DigitalTwinApp);
using namespace inet;


simsignal_t DigitalTwinApp::cbrReqFrameLossSignal_ = registerSignal("cbrReqFrameLossSignal");
simsignal_t DigitalTwinApp::cbrReqFrameDelaySignal_ = registerSignal("cbrReqFrameDelaySignal");
simsignal_t DigitalTwinApp::cbrReqJitterSignal_ = registerSignal("cbrReqJitterSignal");
simsignal_t DigitalTwinApp::cbrReqReceivedThroughtput_ = registerSignal("cbrReqReceivedThroughtputSignal");
simsignal_t DigitalTwinApp::cbrReqReceivedBytesSignal_ = registerSignal("cbrReqReceivedBytesSignal");
simsignal_t DigitalTwinApp::AoIatDTSignal_ = registerSignal("AoIatDTSignal");
simsignal_t DigitalTwinApp::dtComputeTimeSignal_ = registerSignal("dtComputeTimeSignal");

simsignal_t DigitalTwinApp::dtSampleReliabilitySignal_ = registerSignal("dtSampleReliabilitySignal");
simsignal_t DigitalTwinApp::dtReliabilitySignal_ = registerSignal("dtReliabilitySignal");
simsignal_t DigitalTwinApp::dtTimelinessSignal_ = registerSignal("dtTimelinessSignal");


DigitalTwinApp::DigitalTwinApp()
{
    idDTapp_ = ++appDTcounter_;
}

void DigitalTwinApp::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        mInit_ = true;
        numReceived_ = 0;
        recvBytes_ = 0;
        cbrReqRcvdPkt_ = registerSignal("cbrReqRcvdPkt");
        respSize_ = par("responseSize");
        enableVimComputing_ = false;
        activeSubscriber_ = false;
        expectedSamplesPerPeriod_ = par("expectedSamplesPerPeriod");
        targetTimeliness_ = par("targetTimeliness");
        targetReliability_ = par("targetReliability");

        rt_stats_.setName("processingTime");
        EV << "DigitalTwinApp::initialize - initializing DT app having ID=" << idDTapp_ << endl;
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        int port = par("localPort");
        EV << "DigitalTwinApp::initialize - binding to port: local:" << port << endl;

        enableVimComputing_ = par("enableVimComputing").boolValue();
        if (port != -1)
        {
            socket.setOutputGate(gate("socketOut"));
            socket.bind(port);

            destAddress_ = inet::L3AddressResolver().resolve(par("destAddress").stringValue());
            destPort_ = par("destPort");

            if( enableVimComputing_ && par("computingType").intValue() ==  0 )
            {
                computeNodeManager_ = check_and_cast<ComputeNodeManager*>(getParentModule()->getSubmodule("computeNodeManager"));
                computeNodeManager_->registerApp(idDTapp_);
            }

            processingTimer_  = new cMessage("computeMsg");

            std::cout << "DigitalTwinApp::handleMessage - " << par("monitoringDT").stringValue() << ", index: " << par("monitoringDTindex").intValue() << endl;
            int index = par("monitoringDTindex");
            if( index != -1 )
                monitoringDT_ = check_and_cast<MonitoringDT*>(getParentModule()->getSubmodule(par("monitoringDT").stringValue(),index));
            else
                monitoringDT_ = check_and_cast<MonitoringDT*>(getParentModule()->getSubmodule(par("monitoringDT").stringValue()));

            enablePeriodicUpdateGeneration_ = par("enablePeriodicUpdateGeneration");
            if(enablePeriodicUpdateGeneration_)
            {
                EV << "DigitalTwinApp::initialize - enabling generation of periodic updates" << endl;
                generateUpdateTimer_ = new cMessage("generateUpdateMsg");
                updateGenerationPeriod_ = par("updateGenerationPeriod");
                scheduleAfter(simTime()+par("startTime"), generateUpdateTimer_);
            }
        }
    }
}

void DigitalTwinApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if( !strcmp(msg->getName(), "computeMsg" ) )
        {
            EV << "DigitalTwinApp::handleMessage - update ready. Sending it." << endl;
            delete(msg);
            socket.sendTo(respPacket_, destAddress_, destPort_);
            return;
        }
        else if( !strcmp(msg->getName(), "generateUpdateMsg" ) )
        {
            /* TODO handle concurrency between two consecutive updates
             *
             */
            EV << "DigitalTwinApp::handleMessage - update-timer triggered.";
            if( !activeSubscriber_ )
            {
                EV << " No active subscriber. " << endl;
                return;
            }
            EV << " Active subscriber, generating an update." << endl;
            simtime_t processingTime = generateUpdate(0, simTime());
            scheduleAfter(updateGenerationPeriod_, generateUpdateTimer_);
            cMessage* processingTimer = new cMessage("computeMsg");
            scheduleAfter(processingTime, processingTimer);
        }
    }
    // Handling subscription from a DT client
    else if( !strcmp(msg->getName(), "subscription") )
    {
        EV << "DigitalTwinApp::handleMessage - received a subscription request " << endl;
        activeSubscriber_ = true;
        delete(msg);
    }
}

simtime_t DigitalTwinApp::generateUpdate( int idFrame , simtime_t timestamp )
{
    // generate reply
    simtime_t processingTime = 0;
    if( par("enableVimComputing").boolValue() )
    {
        int computingType = par("computingType");
        if( computingType == 0 )
        {
            int numInstructions = par("serviceComplexity").doubleValue() * 1000000;
            processingTime = computeNodeManager_->compute(numInstructions,idDTapp_) ;
            EV << "DigitalTwinApp::handleMessage - requesting an Edge-Based processing time of " << processingTime << " seconds for " << numInstructions << " instructions" <<endl;
        }
        else if( computingType == 1 )
        {
            processingTime = par("extremeEdgeComputingTime");
            EV << "DigitalTwinApp::handleMessage - requesting a Extreme-Edge-Based processing time of " << processingTime << " seconds" <<endl;
        }

        rt_stats_.record(processingTime);
    }

    respPacket_ = new Packet("CBR");
    auto cbr = makeShared<CbrResponse>();
    cbr->setNframes(totFrames_);
    cbr->setIDframe(idFrame);
    cbr->setPayloadTimestamp(timestamp);
    cbr->setPayloadSize(respSize_);
    cbr->setChunkLength(B(respSize_));

    // retrieve samples from the monitoring DT
    simtime_t timestampConsidered = monitoringDT_->getLastPayloadTimestamp();
    int numSamples = monitoringDT_->getNumSamples();
    monitoringDT_->flushSamples();

    EV << simTime() << " - DigitalTwinApp::generateUpdate - reading " << numSamples << "/" << expectedSamplesPerPeriod_ << " samples, having a timestamp of " << timestampConsidered << endl;
    // update statistics and Entanglement information
    // compare the age of the first received sample against the time when the update will be completed
    // in other words, we are evaluating the AoI the sample will have when the computing of the status update is completed

    EV << "\t " << simTime() << " + " << processingTime << " - " << timestampConsidered << " = " << simTime()+processingTime - timestampConsidered ;
    if(simTime()+processingTime - timestampConsidered <= targetTimeliness_)
    {
        emit(dtTimelinessSignal_,1);
        EV << " <= ";
    }
    else
    {
        emit(dtTimelinessSignal_,0);
        EV << " > ";
    }

    EV << targetTimeliness_ << endl;

    double samplesFraction = double(numSamples) / expectedSamplesPerPeriod_;
    emit(dtSampleReliabilitySignal_ , samplesFraction);

    if( samplesFraction >= targetReliability_ )
        emit(dtReliabilitySignal_,1);
    else
        emit(dtReliabilitySignal_,0);

    emit(AoIatDTSignal_,simTime()+processingTime - timestampConsidered);
    emit(dtComputeTimeSignal_,processingTime);
    cbr->setAoiConsidered(timestampConsidered);

    respPacket_->insertAtBack(cbr);

    return processingTime;
}

void DigitalTwinApp::finish()
{
    double lossRate = 0;
    if(totFrames_ > 0)
        lossRate = 1.0-(numReceived_/(totFrames_*1.0));

    emit(cbrReqFrameLossSignal_,lossRate);

    simtime_t elapsedTime = simTime() - getSimulation()->getWarmupPeriod();
    emit( cbrReqReceivedThroughtput_, recvBytes_ / elapsedTime.dbl() );
}



}
