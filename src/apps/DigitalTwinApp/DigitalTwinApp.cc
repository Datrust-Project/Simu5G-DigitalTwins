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

#include "../DigitalTwinApp/DigitalTwinApp.h"

namespace simu5g {
Define_Module(DigitalTwinApp);
using namespace inet;


simsignal_t DigitalTwinApp::cbrReqFrameLossSignal_ = registerSignal("cbrReqFrameLossSignal");
simsignal_t DigitalTwinApp::cbrReqFrameDelaySignal_ = registerSignal("cbrReqFrameDelaySignal");
simsignal_t DigitalTwinApp::cbrReqJitterSignal_ = registerSignal("cbrReqJitterSignal");
simsignal_t DigitalTwinApp::cbrReqReceivedThroughtput_ = registerSignal("cbrReqReceivedThroughtputSignal");
simsignal_t DigitalTwinApp::cbrReqReceivedBytesSignal_ = registerSignal("cbrReqReceivedBytesSignal");
simsignal_t DigitalTwinApp::AoIatDTSignal_ = registerSignal("AoIatDTSignal");

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
                computeNodeManager_->registerApp(idDTapp);
            }

            processingTimer_  = new cMessage("computeMsg");

            std::cout << "DigitalTwinApp::handleMessage - " << par("monitoringDT").stringValue() << ", index: " << par("monitoringDTindex").intValue() << endl;
            int index = par("monitoringDTindex");
            if( index != -1 )
                monitoringDT_ = check_and_cast<CbrReceiver*>(getParentModule()->getSubmodule(par("monitoringDT").stringValue(),index));
            else
                monitoringDT_ = check_and_cast<CbrReceiver*>(getParentModule()->getSubmodule(par("monitoringDT").stringValue()));

            enablePeriodicUpdateGeneration_ = par("enablePeriodicUpdateGeneration");
            if(enablePeriodicUpdateGeneration_)
            {
                EV << "DigitalTwinApp::initialize - enabling generation of periodic updates" << endl;
                generateUpdateTimer_ = new cMessage("generateUpdateMsg");
                updateGenerationPeriod_ = par("updateGenerationPeriod");
                scheduleAfter(simTime()+updateGenerationPeriod_, generateUpdateTimer_);
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
            socket.sendTo(respPacket_, destAddress_, destPort_);
            return;
        }
        else if( !strcmp(msg->getName(), "generateUpdateMsg" ) )
        {
            EV << "DigitalTwinApp::handleMessage - update-timer triggered.";
            if( !activeSubscriber_ )
            {
                EV << " No active subscriber. " << endl;
                return;
            }
            simtime_t processingTime = generateUpdate(0, simTime());
            scheduleAfter(updateGenerationPeriod_, generateUpdateTimer_);
            cMessage* processingTimer = new cMessage("computeMsg");
            scheduleAfter(processingTime, processingTimer);
            EV << " Active subscriber, generating an update." << endl;
        }
    }
    else if( !strcmp(msg->getName(), "subscription") )
    {
        EV << "DigitalTwinApp::handleMessage - received a subscription request " << endl;
        activeSubscriber_ = true;
    }
    else
    {
        Packet* pPacket = check_and_cast<Packet*>(msg);
        auto cbrHeader = pPacket->popAtFront<CbrRequest>();

        numReceived_++;
        totFrames_ = cbrHeader->getNframes(); // XXX this value can be written just once
        int pktSize = (int)cbrHeader->getPayloadSize();

        // just to make sure we do not update recvBytes AND we avoid dividing by 0
        if( simTime() > getSimulation()->getWarmupPeriod() )
        {
            recvBytes_ += pktSize;
            emit( cbrReqReceivedBytesSignal_ , pktSize );
        }

        simtime_t delay = simTime()-cbrHeader->getPayloadTimestamp();
        emit(cbrReqFrameDelaySignal_,delay );

        EV << "DigitalTwinApp::handleMessage - Packet received: FRAME[" << cbrHeader->getIDframe() << "/" << cbrHeader->getNframes() << "] with delay["<< delay << "]" << endl;

        emit(cbrReqRcvdPkt_, (long)cbrHeader->getIDframe());

        simtime_t processingTime = generateUpdate((int)cbrHeader->getIDframe(),cbrHeader->getPayloadTimestamp());

        EV << "DigitalTwinApp::handleMessage - scheduling response in " << processingTime << " seconds." << endl;
        cMessage* processingTimer  = new cMessage("computeMsg");
        scheduleAfter(processingTime, processingTimer);

        delete msg;
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

    simtime_t aoiConsidered = NOW - monitoringDT_->getLastPayloadTimestamp();
    emit(AoIatDTSignal_,aoiConsidered);
    cbr->setAoiConsidered(aoiConsidered);
    //cbr->addTag<CreationTimeTag>()->setCreationTime(simTime());
    respPacket_->insertAtBack(cbr);

    //XXX consider issuing a Timer here
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
