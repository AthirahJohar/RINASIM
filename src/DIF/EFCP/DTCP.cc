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

#include "DTCP.h"

Define_Module(DTCP);

DTCP::DTCP() {
  rxControl = NULL;
  flowControl = NULL;

}

DTCP::~DTCP()
{
  if (rxControl != NULL)
  {
    delete rxControl;
  }
  if (flowControl != NULL)
  {
    delete flowControl;
  }
}


void DTCP::initialize(){
  // TODO Auto-generated constructor stub
dtcpState = new DTCPState();


//TODO A2 based on DTCPState create appropriate components
rxControl = new RXControl();

flowControl = new FlowControl();
}

void DTCP::schedule(DTCPTimers* timer){

}

void DTCP::resetWindowTimer(){
  cancelEvent(windowTimer);
      schedule(windowTimer);
}

void DTCP::updateRcvRtWinEdge(DTPState* dtpState)
{
  dtcpState->updateRcvRtWinEdge(dtpState->getRcvLeftWinEdge());
}

void DTCP::handleMessage(cMessage *msg){
  if(msg->isSelfMessage()){
    /* Self message */
    DTCPTimers* timer = static_cast<DTCPTimers*>(msg);

    switch(timer->getType()){
      case(DTCP_WINDOW_TIMER):{
        handleWindowTimer(static_cast<WindowTimer*>(timer));
      }
    }
  }else{

    /* PANIC!! */
  }

}

void DTCP::handleWindowTimer(WindowTimer* timer){
  resetWindowTimer();
  sendAckPDU();
}



void DTCP::sendAckPDU(){

}

unsigned int DTCP::getNextSndCtrlSeqNum(){
  return rxControl->getNextSndCtrlSeqNum();
}

unsigned int DTCP::getLastCtrlSeqNumRcv(){
  return rxControl->getLastCtrlSeqNumRcv();
}

void DTCP::setLastCtrlSeqnumRec(unsigned int ctrlSeqNum){
  rxControl->setLastCtrlSeqNumRcv(ctrlSeqNum);
}

void DTCP::setSndRtWinEdge(unsigned int sndRtWinEdge)
{
  flowControl->setSendRightWindowEdge(sndRtWinEdge);
}

unsigned int DTCP::getSndRtWinEdge()
{
  if(flowControl != NULL){
  return flowControl->getSendRightWindowEdge();
  }
}

void DTCP::setRcvRtWinEdge(unsigned int rcvRtWinEdge)
{
  flowControl->setRcvRightWindowEdge(rcvRtWinEdge);
}

unsigned int DTCP::getRcvRtWinEdge()
{
  flowControl->getRcvRightWindowEdge();
}

void DTCP::setSndRate(unsigned int sendingRate)
{
  flowControl->setSendingRate(sendingRate);
}
unsigned int DTCP::getSndRate(){
  flowControl->getSendingRate();
}

void DTCP::setRcvRate(unsigned int rcvrRate)
{
  flowControl->setRcvrRate(rcvrRate);
}
unsigned int DTCP::getRcvRate(){
  flowControl->getRcvrRate();
}

unsigned int DTCP::getRcvCredit()
{
  return dtcpState->getRcvCredit();
}


unsigned long DTCP::getSendingTimeUnit()
{
   return flowControl->getSendingTimeUnit();
}


bool DTCP::isSendingRateFullfilled() const
{
  return flowControl->isSendingRateFullfilled();
}

void DTCP::setSendingRateFullfilled(bool rateFullfilled)
{
  flowControl->setSendingRateFullfilled(rateFullfilled);
}
