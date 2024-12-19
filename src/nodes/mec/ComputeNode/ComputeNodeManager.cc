//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "ComputeNodeManager.h"
using namespace std;

namespace simu5g {

Define_Module(ComputeNodeManager);

void ComputeNodeManager::initialize()
{
    // TODO - Generated method body
}

void ComputeNodeManager::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}

void ComputeNodeManager::registerApp(unsigned int idDTapp)
{
    int capacity = par("defaultPerAppCapacity").doubleValue()*1000000;
    EV << simTime() << " - ComputeNodeManager::registerApp - registering a capacity of " << capacity << " operations per second to app " << idDTapp << endl;
    perAppCapacity_[idDTapp] = capacity;
}

void ComputeNodeManager::registerApp(unsigned int idDTapp, unsigned int capacity)
{
    EV << simTime() << " - ComputeNodeManager::registerApp - registering a capacity of " << capacity << " operations per second to app " << idDTapp << endl;
    perAppCapacity_[idDTapp] = capacity;
}

simtime_t ComputeNodeManager::compute(int nOps , unsigned int idDTapp )
{
    EV << simTime() << " - ComputeNodeManager::compute - received a request for " << nOps << " number of operations, from application " << idDTapp << endl;

    if(perAppCapacity_.find(idDTapp) == perAppCapacity_.end())
    {
        throw cRuntimeError("application %d is not registered",idDTapp);
    }
    simtime_t computeTime = double(nOps)/perAppCapacity_[idDTapp];

    computeTime = computeTime + (computeTime *( uniform(0, 0.1)-0.05) );
    EV << "\t available capacity is=" << perAppCapacity_[idDTapp] << endl;
    EV << "\t expected compute time is "<< computeTime << endl;

    return computeTime;
}

} //namespace
