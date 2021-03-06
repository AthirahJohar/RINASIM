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
 * @file RIBd.cc
 * @author Vladimir Vesely (ivesely@fit.vutbr.cz)
 * @date Apr 30, 2014
 * @brief Kind of a Notification Board for DIF
 * @detail
 */

#ifndef RIBDAEMON_H_
#define RIBDAEMON_H_

//Standard libraries
#include <omnetpp.h>
//RINASim libraries
#include "RIBdBase.h"
#include "ExternConsts.h"
#include "RIBdListeners.h"
#include "RINASignals.h"
#include "PDU.h"
#include "IntRoutingUpdate.h"

//Constants
extern const char* MSG_CONGEST;
extern const char* MSG_FLO;
extern const char* CLS_FLOW;
extern const char* MSG_FLOPOSI;
extern const char* MSG_FLONEGA;
extern const int   VAL_DEFINSTANCE;
extern const int   VAL_FLOWPOSI;
extern const int   VAL_FLOWNEGA;
extern const char* VAL_FLREQ;
extern const char* VAL_FLREQPOSI;
extern const char* VAL_FLREQNEGA;
extern const char* MSG_ROUTINGUPDATE;

class RIBd : public RIBdBase {
  public:
    virtual void sendCreateRequestFlow(Flow* flow);
    virtual void sendCreateResponseNegative(Flow* flow);
    virtual void sendCreateResponsePostive(Flow* flow);
    virtual void sendDeleteRequestFlow(Flow* flow);
    virtual void sendStartEnrollmentRequest(EnrollmentObj* obj);
    virtual void sendStartEnrollmentResponse(EnrollmentObj* obj);
    virtual void sendStopEnrollmentRequest(EnrollmentObj* obj);
    virtual void sendStopEnrollmentResponse(EnrollmentObj* obj);
    virtual void sendStartOperationRequest(OperationObj* obj);
    virtual void sendStartOperationResponse(OperationObj* obj);
    virtual void sendDeleteResponseFlow(Flow* flow);
    virtual void receiveData(CDAPMessage* cimsg);
    virtual void receiveCACE(CDAPMessage* msg);
    virtual void sendCACE(CDAPMessage* msg);
    virtual void receiveAllocationRequestFromFai(Flow* flow);
    virtual void receiveCreateFlowPositiveFromRa(Flow* flow);
    virtual void receiveCreateFlowNegativeFromRa(Flow* flow);
    /* Handles information coming from PDUFTG module. */
    virtual void receiveRoutingUpdateFromRouting(IntRoutingUpdate * update);

    virtual void sendCongestionNotification(PDU* pdu);

    void signalizeSendData(CDAPMessage* msg);
    void signalizeCreateRequestFlow(Flow* flow);
    void signalizeDeleteRequestFlow(Flow* flow);
    void signalizeDeleteResponseFlow(Flow* flow);
    void signalizeAllocateResponsePositive(Flow* flow);
    void signalizeAllocateResponseNegative(Flow* flow);
    void signalizeCreateFlow(Flow* flow);
    void signalizeCreateResponseFlowPositive(Flow* flow);
    void signalizeCreateResponseFlowNegative(Flow* flow);
    void signalizeCongestionNotification(CongestionDescriptor* congDesc);

    void signalizeConnectResponsePositive(CDAPMessage* msg);
    void signalizeConnectResponseNegative(CDAPMessage* msg);
    void signalizeConnectRequest(CDAPMessage* msg);
    void signalizeSendCACE(CDAPMessage* msg);

    void signalizeStartEnrollmentRequest(CDAPMessage* msg);
    void signalizeStartEnrollmentResponse(CDAPMessage* msg);
    void signalizeStopEnrollmentRequest(CDAPMessage* msg);
    void signalizeStopEnrollmentResponse(CDAPMessage* msg);
    void signalizeStartOperationRequest(CDAPMessage* msg);
    void signalizeStartOperationResponse(CDAPMessage* msg);

  protected:

    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

    void initCdapBindings();
    void initSignalsAndListeners();

    //Signals
    simsignal_t sigRIBDSendData;
    simsignal_t sigRIBDCreReqFlo;
    simsignal_t sigRIBDDelReqFlo;
    simsignal_t sigRIBDDelResFlo;
    simsignal_t sigRIBDCreResFloPosi;
    simsignal_t sigRIBDCreResFloNega;
    simsignal_t sigRIBDAllocResPosi;
    simsignal_t sigRIBDAllocResNega;
    simsignal_t sigRIBDCreFlow;
    simsignal_t sigRIBDCongNotif;

    simsignal_t sigRIBDStartEnrollReq;
    simsignal_t sigRIBDStartEnrollRes;
    simsignal_t sigRIBDStopEnrollReq;
    simsignal_t sigRIBDStopEnrollRes;
    simsignal_t sigRIBDStartOperationReq;
    simsignal_t sigRIBDStartOperationRes;

    simsignal_t sigRIBDConResPosi;
    simsignal_t sigRIBDConResNega;
    simsignal_t sigRIBDConReq;
    simsignal_t sigRIBDCACESend;

    /* Emit update received signal. */
    //simsignal_t sigRIBDFwdUpdateRecv;
    simsignal_t sigRIBDRoutingUpdateRecv;


    //Listeners
    LisRIBDRcvData*             lisRIBDRcvData;
    LisRIBDCreReq*              lisRIBDCreReq;
    LisRIBDCreReq*              lisRIBDCreReqByForward;
    LisRIBDAllReqFromFai*       lisRIBDAllReqFromFai;
    LisRIBDCreResNega*          lisRIBDCreResNega;
    LisRIBDCreResNega*          lisRIBDCreResNegaFromFa;
    LisRIBDCreResPosi*          lisRIBDCreResPosi;
    LisRIBDCreResPosi*          lisRIBDCreResPosiForward;
    LisRIBDDelReq*              lisRIBDDelReq;
    LisRIBDDelRes*              lisRIBDDelRes;
    LisRIBDCreFloPosi*          lisRIBDCreFloPosi;
    LisRIBDCreFloNega*          lisRIBDCreFloNega;
    LisRIBDCongesNotif*         lisRIBDCongNotif;
    LisRIBDRcvCACE*             lisRIBDRcvCACE;
    LisRIBDRcvEnrollCACE*       lisRIBDRcvEnrollCACE;
    LisRIBDStaEnrolReq*         lisRIBDStaEnrolReq;
    LisRIBDStaEnrolRes*         lisRIBDStaEnrolRes;
    LisRIBDStoEnrolReq*         lisRIBDStoEnrolReq;
    LisRIBDStoEnrolRes*         lisRIBDStoEnrolRes;
    LisRIBDStaOperReq*          lisRIBDStaOperReq;
    LisRIBDStaOperRes*          lisRIBDStaOperRes;

    /* Listen for PDUFTG update messages. */
    LisRIBDRoutingUpdate*       lisRIBDRoutingUpdate;

    void processMConnect(CDAPMessage* msg);
    void processMConnectR(CDAPMessage* msg);

    void processMCreate(CDAPMessage* msg);
    void processMCreateR(CDAPMessage* msg);
    void processMDelete(CDAPMessage* msg);
    void processMDeleteR(CDAPMessage* msg);
    void processMWrite(CDAPMessage* msg);

    void processMStart(CDAPMessage* msg);
    void processMStartR(CDAPMessage* msg);
    void processMStop(CDAPMessage* msg);
    void processMStopR(CDAPMessage* msg);

};

#endif /* RIBDAEMON_H_ */
