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

#include <DTCPSenderAckPolicyTCP.h>
Register_Class(DTCPSenderAckPolicyTCP);

DTCPSenderAckPolicyTCP::DTCPSenderAckPolicyTCP() {
    // TODO Auto-generated constructor stub

}

DTCPSenderAckPolicyTCP::~DTCPSenderAckPolicyTCP() {
    // TODO Auto-generated destructor stub
}

bool DTCPSenderAckPolicyTCP::run(DTPState* dtpState, DTCPState* dtcpState) {
    unsigned int seqNum = ((NAckPDU*)dtpState->getCurrentPdu())->getAckNackSeqNum();

    ackPDU(dtpState, dtcpState, seqNum);
    dtcpState->updateSndLWE(seqNum + 1);

    dtcpState->setClosedWindow(false);

    return false;
}

void DTCPSenderAckPolicyTCP::ackPDU(DTPState* dtpState, DTCPState* dtcpState, unsigned int startSeqNum, unsigned int endSeqNum)
{
    int numOfAcked = 0;
    bool startTrue = false;
    bool notFound = true;
    if(!endSeqNum){
        startTrue = true;
        endSeqNum = startSeqNum;
    }

    std::vector<DTCPRxExpiryTimer*>* rxQ = dtcpState->getRxQ();
    std::vector<DTCPRxExpiryTimer*>::iterator it;

    for (unsigned int index = 0; index < rxQ->size(); )
    {
        DTCPRxExpiryTimer* timer = rxQ->at(index);
        unsigned int seqNum =(timer->getPdu())->getSeqNum();
        unsigned int gap = dtpState->getQoSCube()->getMaxAllowGap();
        //TODO A2 This is weird. Why value from MAX(Ack/Nack, NextAck -1) What does NextAck-1 got to do with it?
        if ((seqNum >= startSeqNum || startTrue) && seqNum <= endSeqNum + gap)
        {
            dtcpState->deleteRxTimer(seqNum);
            numOfAcked++;
            notFound = false;
            continue;
        }
        index++;
    }
    dtcpState->ackRcvd += numOfAcked;
    if(notFound)
        EV << "not found!";
}
