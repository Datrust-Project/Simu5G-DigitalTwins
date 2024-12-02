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

#include <cmath>
#include <inet/common/TimeTag_m.h>
#include "DTclient.h"

namespace simu5g {

Define_Module(DTclient);
using namespace inet;
using namespace std;

simsignal_t DTclient::cbrReqGeneratedThroughtputSignal_ = registerSignal("cbrReqGeneratedThroughtputSignal");
simsignal_t DTclient::cbrReqGeneratedBytesSignal_ = registerSignal("cbrReqGeneratedBytesSignal");
simsignal_t DTclient::cbrReqSentPktSignal_ = registerSignal("cbrReqSentPktSignal");
simsignal_t DTclient::cbrReqServiceTimeSignal_ = registerSignal("cbrReqServiceTimeSignal");


DTclient::DTclient()
{
    initialized_ = false;
    selfSource_ = nullptr;
    selfSender_ = nullptr;
}

DTclient::~DTclient()
{
    cancelAndDelete(selfSource_);
}

void DTclient::initialize(int stage)
{

    cSimpleModule::initialize(stage);
    EV << "CBR Sender initialize: stage " << stage << " - initialize=" << initialized_ << endl;

    if (stage == INITSTAGE_LOCAL)
    {
        selfSource_ = new cMessage("selfSource");
        nframes_ = 0;
        nframesTmp_ = 0;
        iDframe_ = 0;
        timestamp_ = 0;
        size_ = par("PacketSize");
        sampling_time = par("sampling_time");
        localPort_ = par("localPort");
        destPort_ = par("destPort");

        txBytes_ = 0;

        rt_stats_.setName("response_time_vector");

        tempCounter_ = 0;
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        // calculating traffic starting time
        startTime_ = par("startTime");
        finishTime_ = par("finishTime");

        EV << " finish time " << finishTime_ << endl;
        nframes_ = (finishTime_ - startTime_) / sampling_time;

        initTraffic_ = new cMessage("initTraffic");
        initTraffic();
    }
}

void DTclient::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSource"))
        {
            EV << "DTclient::handleMessage - now[" << simTime() << "] <= finish[" << finishTime_ <<"]" <<endl;
            if( simTime() <= finishTime_ || finishTime_ == 0 )
                sendDTRequest();
        }
        else
            initTraffic();
    }
    else // receiving response
    {
        handleResponse(msg);
    }
}

void DTclient::initTraffic()
{
    std::string destAddress = par("destAddress").stringValue();
    cModule* destModule = findModuleByPath(par("destAddress").stringValue());
    if (destModule == nullptr)
    {
        // this might happen when users are created dynamically
        EV << simTime() << "DTclient::initTraffic - destination " << destAddress << " not found" << endl;

        simtime_t offset = 0.01; // TODO check value
        scheduleAt(simTime()+offset, initTraffic_);
        EV << simTime() << "DTclient::initTraffic - the node will retry to initialize traffic in " << offset << " seconds " << endl;
    }
    else
    {
        delete initTraffic_;
        std::cout << par("destAddress").stringValue() << std::endl;
        destAddress_ = inet::L3AddressResolver().resolve(par("destAddress").stringValue());

        // set primary and secondary address
        socket.setOutputGate(gate("socketOut"));
        socket.bind(localPort_);

        int tos = par("tos");
        if (tos != -1)
            socket.setTos(tos);

        EV << simTime() << "DTclient::initialize - binding to port: local:" << localPort_ << " , dest: " << destAddress_.str() << ":" << destPort_ << endl;

        // calculating traffic starting time
        simtime_t startTime = par("startTime");

        scheduleAt(simTime()+startTime, selfSource_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void DTclient::sendDTRequest()
{
    Packet* packet = new Packet("subscription");
    auto dtReq = makeShared<DTrequestMessage>();
    dtReq->setNframes(nframes_);
    dtReq->setIDframe(iDframe_++);
    dtReq->setPayloadTimestamp(simTime());
    dtReq->setPayloadSize(size_);
    dtReq->setChunkLength(B(size_));
    dtReq->addTag<CreationTimeTag>()->setCreationTime(simTime());
    dtReq->setReqType("subscription");

    packet->insertAtBack(dtReq);

    emit(cbrReqGeneratedBytesSignal_,size_);

    if( simTime() > getSimulation()->getWarmupPeriod() )
    {
        txBytes_ += size_;
    }

    EV << "DTclient::sendDTRequest - sending subscription to DT service" << endl;
    socket.sendTo(packet, destAddress_, destPort_);
}


void DTclient::handleResponse(cMessage *msg)
{
    Packet* pPacket = check_and_cast<Packet*>(msg);
    auto cbrHeader = pPacket->popAtFront<CbrResponse>();

    simtime_t rtt = simTime()-cbrHeader->getPayloadTimestamp();
    emit(cbrReqServiceTimeSignal_,rtt );
    rt_stats_.record(rtt);

    EV << "DTclient::handleMessage - response received after " << rtt << " seconds." << endl;

    delete msg;
}




void DTclient::finish()
{
    simtime_t elapsedTime = simTime() - getSimulation()->getWarmupPeriod();
    emit( cbrReqGeneratedThroughtputSignal_, txBytes_ / elapsedTime.dbl() );
}

}
