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

#ifndef __RINA_AEPING_H_
#define __RINA_AEPING_H_

//Standard libraries
#include <omnetpp.h>
//RINASim libraries
#include "AE.h"

class AEPing : public AE
{
    //Consts
    const char* TIM_START           = "StartCommunication";
    const char* TIM_STOP            = "StopCommunication";
    const char* MSG_PING            = "PING-";
    const char* PAR_START           = "startAt";
    const char* PAR_STOP            = "stopAt";
    const char* PAR_PING            = "pingAt";
    const char* PAR_RATE            = "rate";
    const char* PAR_SIZE            = "size";
    const char* PAR_DSTAPNAME       = "dstApName";
    const char* PAR_DSTAPINSTANCE   = "dstApInstance";
    const char* PAR_DSTAENAME       = "dstAeName";
    const char* PAR_DSTAEINSTANCE   = "dstAeInstance";
    const char* VAL_MODULEPATH      = "getFullPath()";

  public:
    AEPing();
    virtual ~AEPing();

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

    void handleSelfMessage(cMessage* msg);

  private:
    std::string myPath;

    std::string dstApName;
    std::string dstApInstance;
    std::string dstAeName;
    std::string dstAeInstance;

    simtime_t startAt;
    simtime_t stopAt;
    simtime_t pingAt;
    int rate;
    unsigned int size;

    void prepareAllocateRequest();
    void preparePing();
    void prepareDeallocateRequest();

    virtual void processMRead(CDAPMessage* msg);
    virtual void processMReadR(CDAPMessage* msg);

};

#endif
