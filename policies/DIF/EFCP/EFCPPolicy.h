//
// Copyright © 2014 - 2015 PRISTINE Consortium (http://ict-pristine.eu)
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
 * @file EFCPPolicy.h
 * @author Marcel Marek (imarek@fit.vutbr.cz)
 * @date May 3, 2015
 * @brief
 * @detail
 */


#ifndef EFCPPOLICY_H_
#define EFCPPOLICY_H_

#include <omnetpp.h>

#include "DTPState.h"
#include "DTCPState.h"

class EFCPPolicy : public cSimpleModule
{
  public:
    EFCPPolicy();
    virtual ~EFCPPolicy();
    virtual bool call(DTPState* dtpState, DTCPState* dtcpState);

  protected:
    virtual void initialize(){};
    virtual void handleMessage(cMessage* msg){};
    virtual void defaultAction(DTPState* dtpState, DTCPState* dtcpState) = 0;
    virtual bool run(DTPState* dtpState, DTCPState* dtcpState) = 0;
};

#endif /* EFCPPOLICY_H_ */
