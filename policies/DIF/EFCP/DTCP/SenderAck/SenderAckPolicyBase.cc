//
// Copyright © 2014 PRISTINE Consortium (http://ict-pristine.eu)
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
/**
 * @file SenderAckPolicyBase.cc
 * @author Marcel Marek (imarek@fit.vutbr.cz)
 * @date Jan 7, 2015
 * @brief
 * @detail
 */

#include "SenderAckPolicyBase.h"
#include "DTCP.h"

SenderAckPolicyBase::SenderAckPolicyBase()
{


}

SenderAckPolicyBase::~SenderAckPolicyBase()
{

}

void SenderAckPolicyBase::defaultAction(DTPState* dtpState, DTCPState* dtcpState)
{
  DTCP* dtcp = (DTCP*)getModuleByPath((std::string(".^.") + std::string(MOD_DTCP)).c_str());
  /* Default */
  unsigned int seqNum = ((NAckPDU*)dtpState->getCurrentPdu())->getAckNackSeqNum();
  dtcp->ackPDU(seqNum);

  //update SendLeftWindowEdge
  dtcpState->updateSndLWE(seqNum + 1);

  /* End default */
}
