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

#ifndef __SIMU5G_DIGITALTWINS_COMPUTENODEMANAGER_H_
#define __SIMU5G_DIGITALTWINS_COMPUTENODEMANAGER_H_

#include <omnetpp.h>
#include <map>

using namespace omnetpp;

namespace simu5g {

/**
 * TODO - Generated class
 */
class ComputeNodeManager : public cSimpleModule
{
    std::map<unsigned int,int> perAppCapacity_;
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

  public:
    simtime_t compute(int nOps, unsigned int idDTapp);
    void registerApp(unsigned int idDTapp);
};

} //namespace

#endif
