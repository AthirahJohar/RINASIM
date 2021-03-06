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

#include "RIBd.h"

const char* MSG_CONGEST         = "Congestion";
const char* MSG_FLO             = "Flow";
const char* MSG_FLOPOSI         = "Flow+";
const char* MSG_FLONEGA         = "Flow-";
const char* CLS_FLOW            = "Flow";
const int   VAL_DEFINSTANCE     = -1;
const int   VAL_FLOWPOSI        = 1;
const int   VAL_FLOWNEGA        = 0;
const char* VAL_FLREQ           = "Request  ";
const char* VAL_FLREQPOSI       = "Response+  ";
const char* VAL_FLREQNEGA       = "Response-  ";
const char* MSG_ROUTINGUPDATE       = "RoutingUpdate";

Define_Module(RIBd);

void RIBd::initialize() {
    //Init signals and listeners
    initSignalsAndListeners();
    //Init MyAddress
    initMyAddress();
}

void RIBd::handleMessage(cMessage *msg) {

}

void RIBd::sendCreateRequestFlow(Flow* flow) {
    Enter_Method("sendCreateRequestFlow()");

    //Prepare M_CREATE Flow
    CDAP_M_Create* mcref = new CDAP_M_Create(MSG_FLO);

    //Prepare object
    std::ostringstream os;
    os << flow->getFlowName();
    object_t flowobj;
    flowobj.objectClass = flow->getClassName();
    flowobj.objectName = os.str();
    flowobj.objectVal = flow;
    //TODO: Vesely - Assign appropriate values
    flowobj.objectInstance = VAL_DEFINSTANCE;
    mcref->setObject(flowobj);

    //Append destination address for RMT "routing"
    mcref->setDstAddr(flow->getDstNeighbor());

    //Generate InvokeId
    if (!flow->getAllocInvokeId())
        flow->setAllocInvokeId(getNewInvokeId());
    mcref->setInvokeID(flow->getAllocInvokeId());

    //Send it
    signalizeSendData(mcref);
}

void RIBd::processMCreate(CDAPMessage* msg) {
    CDAP_M_Create* msg1 = check_and_cast<CDAP_M_Create*>(msg);

    EV << "Received M_Create";
    object_t object = msg1->getObject();
    EV << " with object '" << object.objectClass << "'" << endl;

    //CreateRequest Flow
    if (dynamic_cast<Flow*>(object.objectVal)) {
        Flow* fl = (check_and_cast<Flow*>(object.objectVal))->dup();
        //EV << fl->info();
        fl->swapFlow();
        //EV << "\n===========\n" << fl->info();
        signalizeCreateRequestFlow(fl);
    }
}

void RIBd::sendDeleteRequestFlow(Flow* flow) {
    Enter_Method("sendDeleteRequestFlow()");

    //Prepare M_CREATE Flow
    CDAP_M_Delete* mdereqf = new CDAP_M_Delete(MSG_FLO);

    //Prepare object
    std::ostringstream os;
    os << flow->getFlowName();
    object_t flowobj;
    flowobj.objectClass = flow->getClassName();
    flowobj.objectName = os.str();
    flowobj.objectVal = flow;
    //TODO: Vesely - Assign appropriate values
    flowobj.objectInstance = VAL_DEFINSTANCE;
    mdereqf->setObject(flowobj);

    //Append destination address for RMT "routing"
    mdereqf->setDstAddr(flow->getDstAddr());

    //Generate InvokeId
    if (!flow->getDeallocInvokeId())
        flow->setDeallocInvokeId(getNewInvokeId());
    mdereqf->setInvokeID(flow->getDeallocInvokeId());

    //Send it
    signalizeSendData(mdereqf);
}

void RIBd::processMCreateR(CDAPMessage* msg) {
    CDAP_M_Create_R* msg1 = check_and_cast<CDAP_M_Create_R*>(msg);

    EV << "Received M_Create_R";
    object_t object = msg1->getObject();
    EV << " with object '" << object.objectClass << "'" << endl;

    //CreateResponseFlow
    if (dynamic_cast<Flow*>(object.objectVal)) {
        Flow* flow = (check_and_cast<Flow*>(object.objectVal))->dup();
        flow->swapFlow();
        //Positive response
        if (!msg1->getResult().resultValue) {
            signalizeCreateResponseFlowPositive(flow);
        }
        //Negative response
        else
            signalizeCreateResponseFlowNegative(flow);
    }
}

void RIBd::receiveData(CDAPMessage* msg) {
    Enter_Method("receiveData()");
    //M_CREATE_Request
    if (dynamic_cast<CDAP_M_Create*>(msg)) {
        processMCreate(msg);
    }
    //M_CREATE_Response
    else if (dynamic_cast<CDAP_M_Create_R*>(msg)) {
        processMCreateR(msg);
    }
    //M_DELETE_Request
    else if (dynamic_cast<CDAP_M_Delete*>(msg)) {
        processMDelete(msg);
    }
    //M_DELETE_Request
    else if (dynamic_cast<CDAP_M_Delete_R*>(msg)) {
        processMDeleteR(msg);
    }
    //M_WRITE_Request
    else if (dynamic_cast<CDAP_M_Write*>(msg)) {
        processMWrite(msg);
    }
    //M_START_Request
    else if (dynamic_cast<CDAP_M_Start*>(msg)) {
        processMStart(msg);
    }
    //M_START_Response
    else if (dynamic_cast<CDAP_M_Start_R*>(msg)) {
        processMStartR(msg);
    }
    //M_Stop_Request
    else if (dynamic_cast<CDAP_M_Stop*>(msg)) {
        processMStop(msg);
    }
    //M_Stop_Response
    else if (dynamic_cast<CDAP_M_Stop_R*>(msg)) {
        processMStopR(msg);
    }

    delete msg;
}

void RIBd::receiveCACE(CDAPMessage* msg) {
    Enter_Method("receiveCACE()");

    //M_CONNECT_Request
    if (dynamic_cast<CDAP_M_Connect*>(msg)) {
        processMConnect(msg);
    }
    //M_CONNECT_Response
    else if (dynamic_cast<CDAP_M_Connect_R*>(msg)) {
        processMConnectR(msg);
    }

    delete msg;
}

void RIBd::initSignalsAndListeners() {
    cModule* catcher1 = this->getParentModule();
    cModule* catcher2 = this->getParentModule()->getParentModule();
    cModule* catcher3 = this->getParentModule()->getParentModule()->getParentModule();

    //Signals that this module is emitting
    sigRIBDSendData      = registerSignal(SIG_RIBD_DataSend);
    sigRIBDCreReqFlo     = registerSignal(SIG_RIBD_CreateRequestFlow);
    sigRIBDDelReqFlo     = registerSignal(SIG_RIBD_DeleteRequestFlow);
    sigRIBDDelResFlo     = registerSignal(SIG_RIBD_DeleteResponseFlow);
    sigRIBDAllocResPosi  = registerSignal(SIG_AERIBD_AllocateResponsePositive);
    sigRIBDAllocResNega  = registerSignal(SIG_AERIBD_AllocateResponseNegative);
    sigRIBDCreFlow       = registerSignal(SIG_RIBD_CreateFlow);
    sigRIBDCreResFloPosi = registerSignal(SIG_RIBD_CreateFlowResponsePositive);
    sigRIBDCreResFloNega = registerSignal(SIG_RIBD_CreateFlowResponseNegative);
   // sigRIBDFwdUpdateRecv = registerSignal(SIG_RIBD_ForwardingUpdateReceived);
    sigRIBDRoutingUpdateRecv = registerSignal(SIG_RIBD_RoutingUpdateReceived);
    sigRIBDCongNotif     = registerSignal(SIG_RIBD_CongestionNotification);

    sigRIBDStartEnrollReq = registerSignal(SIG_RIBD_StartEnrollmentRequest);
    sigRIBDStartEnrollRes = registerSignal(SIG_RIBD_StartEnrollmentResponse);
    sigRIBDStopEnrollReq  = registerSignal(SIG_RIBD_StopEnrollmentRequest);
    sigRIBDStopEnrollRes  = registerSignal(SIG_RIBD_StopEnrollmentResponse);
    sigRIBDStartOperationReq = registerSignal(SIG_RIBD_StartOperationRequest);
    sigRIBDStartOperationRes = registerSignal(SIG_RIBD_StartOperationResponse);

    sigRIBDConResPosi    = registerSignal(SIG_RIBD_ConnectionResponsePositive);
    sigRIBDConResNega    = registerSignal(SIG_RIBD_ConnectionResponseNegative);
    sigRIBDConReq        = registerSignal(SIG_RIBD_ConnectionRequest);
    sigRIBDCACESend      = registerSignal(SIG_RIBD_CACESend);


    //Signals that this module is processing

    lisRIBDCreReqByForward = new LisRIBDCreReq(this);
    catcher2->subscribe(SIG_FA_CreateFlowRequestForward, lisRIBDCreReqByForward);
    lisRIBDCreReq = new LisRIBDCreReq(this);
    catcher2->subscribe(SIG_FAI_CreateFlowRequest, lisRIBDCreReq);

    lisRIBDDelReq = new LisRIBDDelReq(this);
    catcher2->subscribe(SIG_FAI_DeleteFlowRequest, lisRIBDDelReq);
    lisRIBDDelRes = new LisRIBDDelRes(this);
    catcher2->subscribe(SIG_FAI_DeleteFlowResponse, lisRIBDDelRes);

    lisRIBDCreResNegaFromFa = new LisRIBDCreResNega(this);
    catcher2->subscribe(SIG_FA_CreateFlowResponseNegative, lisRIBDCreResNegaFromFa);
    lisRIBDCreResNega = new LisRIBDCreResNega(this);
    catcher2->subscribe(SIG_FAI_CreateFlowResponseNegative, lisRIBDCreResNega);

    lisRIBDCreResPosi = new LisRIBDCreResPosi(this);
    catcher2->subscribe(SIG_FAI_CreateFlowResponsePositive, lisRIBDCreResPosi);
    lisRIBDCreResPosiForward = new LisRIBDCreResPosi(this);
    catcher2->subscribe(SIG_FA_CreateFlowResponseForward, lisRIBDCreResPosiForward);

    lisRIBDRcvData = new LisRIBDRcvData(this);
    catcher1->subscribe(SIG_CDAP_DateReceive, lisRIBDRcvData);

    lisRIBDAllReqFromFai = new LisRIBDAllReqFromFai(this);
    catcher3->subscribe(SIG_FAI_AllocateRequest, lisRIBDAllReqFromFai);

    lisRIBDCreFloPosi = new LisRIBDCreFloPosi(this);
    catcher2->subscribe(SIG_RA_CreateFlowPositive, lisRIBDCreFloPosi);
    lisRIBDCreFloNega = new LisRIBDCreFloNega(this);
    catcher2->subscribe(SIG_RA_CreateFlowNegative, lisRIBDCreFloNega);

    lisRIBDRoutingUpdate = new LisRIBDRoutingUpdate(this);
    catcher2->subscribe(SIG_RIBD_RoutingUpdate, lisRIBDRoutingUpdate);

    lisRIBDCongNotif = new LisRIBDCongesNotif(this);
    catcher2->subscribe(SIG_RA_InvokeSlowdown, lisRIBDCongNotif);


    lisRIBDRcvCACE = new LisRIBDRcvCACE(this);
    catcher1->subscribe(SIG_CACE_DataReceive, lisRIBDRcvCACE);

    lisRIBDRcvEnrollCACE = new LisRIBDRcvEnrollCACE(this);
    catcher2->subscribe(SIG_ENROLLMENT_CACEDataSend, lisRIBDRcvEnrollCACE);

    lisRIBDStaEnrolReq = new LisRIBDStaEnrolReq(this);
    catcher2->subscribe(SIG_ENROLLMENT_StartEnrollmentRequest, lisRIBDStaEnrolReq);

    lisRIBDStaEnrolRes = new LisRIBDStaEnrolRes(this);
    catcher2->subscribe(SIG_ENROLLMENT_StartEnrollmentResponse, lisRIBDStaEnrolRes);

    lisRIBDStoEnrolReq = new LisRIBDStoEnrolReq(this);
    catcher2->subscribe(SIG_ENROLLMENT_StopEnrollmentRequest, lisRIBDStoEnrolReq);

    lisRIBDStoEnrolRes = new LisRIBDStoEnrolRes(this);
    catcher2->subscribe(SIG_ENROLLMENT_StopEnrollmentResponse, lisRIBDStoEnrolRes);

    lisRIBDStaOperReq = new LisRIBDStaOperReq(this);
    catcher2->subscribe(SIG_ENROLLMENT_StartOperationRequest, lisRIBDStaOperReq);

    lisRIBDStaOperRes = new LisRIBDStaOperRes(this);
    catcher2->subscribe(SIG_ENROLLMENT_StartOperationResponse, lisRIBDStaOperRes);

}

void RIBd::receiveAllocationRequestFromFai(Flow* flow) {
    Enter_Method("receiveAllocationRequestFromFai()");
    if (flow->isManagementFlowLocalToIPCP())
    {
        receiveCreateFlowPositiveFromRa(flow);
    }
    else {
        //Execute flow allocate
        signalizeCreateFlow(flow);
    }
}

void RIBd::sendCreateResponseNegative(Flow* flow) {
    Enter_Method("sendCreateResponseFlowNegative()");

    //Prepare M_CREATE_R Flow-
    CDAP_M_Create_R* mcref = new CDAP_M_Create_R(MSG_FLONEGA);

    //Prepare object
    std::ostringstream os;
    os << flow->getFlowName();
    object_t flowobj;
    flowobj.objectClass = flow->getClassName();
    flowobj.objectName = os.str();
    flowobj.objectVal = flow;
    //TODO: Vesely - Assign appropriate values
    flowobj.objectInstance = VAL_DEFINSTANCE;
    mcref->setObject(flowobj);

    //Prepare result object
    result_t resultobj;
    resultobj.resultValue = R_FAIL;
    mcref->setResult(resultobj);

    //Generate InvokeId
    mcref->setInvokeID(flow->getAllocInvokeId());

    //Append destination address for RMT "routing"
    mcref->setDstAddr(flow->getDstAddr());

    //Send it
    signalizeSendData(mcref);
}

void RIBd::sendCreateResponsePostive(Flow* flow) {
    Enter_Method("sendCreateResponseFlowPositive()");

    //Prepare M_CREATE_R Flow+
    CDAP_M_Create_R* mcref = new CDAP_M_Create_R(MSG_FLOPOSI);

    //Prepare object
    std::ostringstream os;
    os << flow->getFlowName();
    object_t flowobj;
    flowobj.objectClass = flow->getClassName();
    flowobj.objectName = os.str();
    flowobj.objectVal = flow;
    //TODO: Vesely - Assign appropriate values
    flowobj.objectInstance = VAL_DEFINSTANCE;
    mcref->setObject(flowobj);

    //Prepare result object
    result_t resultobj;
    resultobj.resultValue = R_SUCCESS;
    mcref->setResult(resultobj);

    //Generate InvokeId
    mcref->setInvokeID(flow->getAllocInvokeId());

    //Append destination address for RMT "routing"
    mcref->setDstAddr(flow->getDstAddr());

    //Send it
    signalizeSendData(mcref);
}

void RIBd::signalizeSendData(CDAPMessage* msg) {
    //Check dstAddress
    if (msg->getDstAddr() == Address::UNSPECIFIED_ADDRESS) {
        EV << "Destination address cannot be UNSPECIFIED!" << endl;
        return;
    }

    msg->setBitLength(msg->getBitLength() + msg->getHeaderBitLength());
    //Pass message to CDAP
    EV << "Emits SendData signal for message " << msg->getName() << endl;
    emit(sigRIBDSendData, msg);
}

void RIBd::signalizeAllocateResponsePositive(Flow* flow) {
    EV << "Emits AllocateResponsePositive signal for flow" << endl;
    emit(sigRIBDAllocResPosi, flow);
}

void RIBd::signalizeCreateRequestFlow(Flow* flow) {
    EV << "Emits CreateRequestFlow signal for flow" << endl;
    emit(sigRIBDCreReqFlo, flow);
}

void RIBd::signalizeCreateResponseFlowPositive(Flow* flow) {
    EV << "Emits CreateResponsetFlowPositive signal for flow" << endl;
    emit(sigRIBDCreResFloPosi, flow);
}

void RIBd::signalizeCreateResponseFlowNegative(Flow* flow) {
    EV << "Emits CreateResponsetFlowNegative signal for flow" << endl;
    emit(sigRIBDCreResFloNega, flow);
}

void RIBd::signalizeCreateFlow(Flow* flow) {
    EV << "Emits CreateFlow signal for flow" << endl;
    emit(sigRIBDCreFlow, flow);
}

void RIBd::processMDelete(CDAPMessage* msg) {
    CDAP_M_Delete* msg1 = check_and_cast<CDAP_M_Delete*>(msg);

    EV << "Received M_Delete";
    object_t object = msg1->getObject();
    EV << " with object '" << object.objectClass << "'" << endl;

    //DeleteRequest Flow
    if (dynamic_cast<Flow*>(object.objectVal)) {
        Flow* fl = (check_and_cast<Flow*>(object.objectVal))->dup();
        fl->swapFlow();
        signalizeDeleteRequestFlow(fl);
    }

}

void RIBd::signalizeDeleteRequestFlow(Flow* flow) {
    EV << "Emits DeleteRequestFlow signal for flow" << endl;
    emit(sigRIBDDelReqFlo, flow);
}

void RIBd::sendDeleteResponseFlow(Flow* flow) {
    Enter_Method("sendDeleteResponseFlow()");

    //Prepare M_CREATE_R Flow+
    CDAP_M_Delete_R* mderesf = new CDAP_M_Delete_R(MSG_FLOPOSI);

    //Prepare object
    std::ostringstream os;
    os << flow->getFlowName();
    object_t flowobj;
    flowobj.objectClass = flow->getClassName();
    flowobj.objectName = os.str();
    flowobj.objectVal = flow;
    //TODO: Vesely - Assign appropriate values
    flowobj.objectInstance = VAL_DEFINSTANCE;
    mderesf->setObject(flowobj);

    //Prepare result object
    result_t resultobj;
    resultobj.resultValue = R_SUCCESS;
    mderesf->setResult(resultobj);

    //Generate InvokeId
    mderesf->setInvokeID(flow->getDeallocInvokeId());

    //Append destination address for RMT "routing"
    mderesf->setDstAddr(flow->getDstAddr());

    //Send it
    signalizeSendData(mderesf);

}

void RIBd::signalizeDeleteResponseFlow(Flow* flow) {
    EV << "Emits DeleteResponseFlow signal for flow" << endl;
    emit(sigRIBDDelResFlo, flow);
}

void RIBd::receiveCreateFlowPositiveFromRa(Flow* flow) {
    signalizeAllocateResponsePositive(flow);
}

void RIBd::receiveCreateFlowNegativeFromRa(Flow* flow) {
    signalizeAllocateResponseNegative(flow);
}

void RIBd::signalizeAllocateResponseNegative(Flow* flow) {
    EV << "Emits AllocateResponseNegative signal for flow" << endl;
    emit(sigRIBDAllocResNega, flow);
}

void RIBd::processMDeleteR(CDAPMessage* msg) {
    CDAP_M_Delete_R* msg1 = check_and_cast<CDAP_M_Delete_R*>(msg);

    EV << "Received M_Delete_R";
    object_t object = msg1->getObject();
    EV << " with object '" << object.objectClass << "'" << endl;

    //DeleteResponseFlow
    if (dynamic_cast<Flow*>(object.objectVal)) {
        Flow* flow = (check_and_cast<Flow*>(object.objectVal))->dup();
        flow->swapFlow();
        signalizeDeleteResponseFlow(flow);
    }

}

void RIBd::sendCongestionNotification(PDU* pdu) {
    Enter_Method("sendCongestionNotification()");

    //Prepare M_START ConDescr
    CDAP_M_Start* mstarcon = new CDAP_M_Start(MSG_CONGEST);
    CongestionDescriptor* conDesc = new CongestionDescriptor(pdu->getConnId().getDstCepId(), pdu->getConnId().getSrcCepId(), pdu->getConnId().getQoSId());
    //Prepare object
    std::ostringstream os;
    os << conDesc->getCongesDescrName();
    object_t condesobj;
    condesobj.objectClass = conDesc->getClassName();
    condesobj.objectName = os.str();
    condesobj.objectVal = conDesc;
    //TODO: Vesely - Assign appropriate values
    condesobj.objectInstance = VAL_DEFINSTANCE;
    mstarcon->setObject(condesobj);

    //Generate InvokeId
    mstarcon->setInvokeID(DONTCARE_INVOKEID);

    //Append destination address for RMT "routing"
    mstarcon->setDstAddr(pdu->getSrcAddr());

    //Send it
    signalizeSendData(mstarcon);
}

void RIBd::processMWrite(CDAPMessage* msg)
{
    CDAP_M_Write * msg1 = check_and_cast<CDAP_M_Write *>(msg);

    EV << "Received M_Write";
    object_t object = msg1->getObject();
    EV << " with object '" << object.objectClass << "'" << endl;

    //CreateRequest Flow
    if (dynamic_cast<IntRoutingUpdate *>(object.objectVal))
    {
        IntRoutingUpdate * update = (check_and_cast<IntRoutingUpdate *>(object.objectVal));

        /* Signal that an update obj has been received. */
        emit(sigRIBDRoutingUpdateRecv, update);
    }
}

void RIBd::receiveRoutingUpdateFromRouting(IntRoutingUpdate * info)
{
    EV << getFullPath() << " Forwarding update to send to " << info->getDestination();

    /* Emits the CDAP message. */

    CDAP_M_Write * cdapm = new CDAP_M_Write(MSG_ROUTINGUPDATE);
    std::ostringstream os;
    object_t flowobj;

    /* Prepare the object to send. */

    os << "RoutingUpdateTo" << info->getDestination();

    flowobj.objectClass = info->getClassName();
    flowobj.objectName  = os.str();
    flowobj.objectVal   = info;
    //TODO: Vesely - Assign appropriate values
    flowobj.objectInstance = VAL_DEFINSTANCE;

    cdapm->setObject(flowobj);

    //TODO: Vesely - Work more on InvokeId

    /* This message will be sent to... */
    cdapm->setDstAddr(info->getDestination());

    /* Finally order to send the data... */
    signalizeSendData(cdapm);
}

void RIBd::signalizeCongestionNotification(CongestionDescriptor* congDesc) {
    EV << "Emits CongestionNotification" << endl;
    emit(sigRIBDCongNotif, congDesc);
}

void RIBd::signalizeConnectResponsePositive(CDAPMessage* msg) {
    EV << "Emits ConnectResponsePositive to enrollment" << endl;
    emit(sigRIBDConResPosi, msg);
}

void RIBd::signalizeConnectResponseNegative(CDAPMessage* msg) {
    EV << "Emits ConnectResponseNegative to enrollment" << endl;
    emit(sigRIBDConResNega, msg);
}

void RIBd::signalizeConnectRequest(CDAPMessage* msg) {
    EV << "Emits ConnectRequest to enrollment" << endl;
    emit(sigRIBDConReq, msg);
}

void RIBd::signalizeSendCACE(CDAPMessage* msg) {
    EV << "Emits CACE data to CACE module" << endl;
    emit(sigRIBDCACESend, msg);
}

void RIBd::signalizeStartEnrollmentRequest(CDAPMessage* msg) {
    EV << "Emits StartEnrollmentRequest to enrollment" << endl;
    emit(sigRIBDStartEnrollReq, msg);
}

void RIBd::signalizeStartEnrollmentResponse(CDAPMessage* msg) {
    EV << "Emits StartEnrollmentResponse to enrollment" << endl;
    emit(sigRIBDStartEnrollRes, msg);
}

void RIBd::signalizeStopEnrollmentRequest(CDAPMessage* msg) {
    EV << "Emits StopEnrollmentRequest to enrollment" << endl;
    emit(sigRIBDStopEnrollReq, msg);
}

void RIBd::signalizeStopEnrollmentResponse(CDAPMessage* msg) {
    EV << "Emits StopEnrollmentResponse to enrollment" << endl;
    emit(sigRIBDStopEnrollRes, msg);
}

void RIBd::signalizeStartOperationRequest(CDAPMessage* msg) {
    EV << "Emits StartOperationRequest to enrollment" << endl;
    emit(sigRIBDStartOperationReq, msg);
}

void RIBd::signalizeStartOperationResponse(CDAPMessage* msg) {
    EV << "Emits StartOperationResponse to enrollment" << endl;
    emit(sigRIBDStartOperationRes, msg);
}

void RIBd::sendStartEnrollmentRequest(EnrollmentObj* obj) {
    Enter_Method("sendStartEnrollmentRequest()");

    CDAP_M_Start* msg = new CDAP_M_Start("Start Enrollment");

    //TODO: assign appropriate values
    std::ostringstream os;
    os << "Enrollment";
    object_t enrollobj;
    enrollobj.objectClass = obj->getClassName();
    enrollobj.objectName = os.str();
    enrollobj.objectVal = obj;
    enrollobj.objectInstance = VAL_DEFINSTANCE;
    msg->setObject(enrollobj);
    msg->setOpCode(M_START);

    //TODO: check and rework generate invoke id
    //msg->setInvokeID(getNewInvokeId());

    msg->setDstAddr(obj->getDstAddress());

    signalizeSendData(msg);
}

void RIBd::sendStartEnrollmentResponse(EnrollmentObj* obj) {
    Enter_Method("sendStartEnrollmentResponse()");

    CDAP_M_Start_R* msg = new CDAP_M_Start_R("Start_R Enrollment");

    //TODO: assign appropriate values
    std::ostringstream os;
    os << "Enrollment";
    object_t enrollobj;
    enrollobj.objectClass = obj->getClassName();
    enrollobj.objectName = os.str();
    enrollobj.objectVal = obj;
    enrollobj.objectInstance = VAL_DEFINSTANCE;
    msg->setObject(enrollobj);
    msg->setOpCode(M_START_R);

    //TODO: check and rework generate invoke id
    //msg->setInvokeID(getNewInvokeId());

    msg->setDstAddr(obj->getDstAddress());

    signalizeSendData(msg);
}

void RIBd::sendStopEnrollmentRequest(EnrollmentObj* obj) {
    Enter_Method("sendStopEnrollmentRequest()");

    CDAP_M_Stop* msg = new CDAP_M_Stop("Stop Enrollment");

    //TODO: assign appropriate values
    std::ostringstream os;
    os << "Enrollment";
    object_t enrollobj;
    enrollobj.objectClass = obj->getClassName();
    enrollobj.objectName = os.str();
    enrollobj.objectVal = obj;
    enrollobj.objectInstance = VAL_DEFINSTANCE;
    msg->setObject(enrollobj);
    msg->setOpCode(M_STOP);

    //TODO: check and rework generate invoke id
    //msg->setInvokeID(getNewInvokeId());

    msg->setDstAddr(obj->getDstAddress());

    signalizeSendData(msg);
}

void RIBd::sendStopEnrollmentResponse(EnrollmentObj* obj) {
    Enter_Method("sendStopEnrollmentResponse()");

    CDAP_M_Stop_R* msg = new CDAP_M_Stop_R("Stop_R Enrollment");

    //TODO: assign appropriate values
    std::ostringstream os;
    os << "Enrollment";
    object_t enrollobj;
    enrollobj.objectClass = obj->getClassName();
    enrollobj.objectName = os.str();
    enrollobj.objectVal = obj;
    enrollobj.objectInstance = VAL_DEFINSTANCE;
    msg->setObject(enrollobj);
    msg->setOpCode(M_STOP_R);

    //TODO: check and rework generate invoke id
    //msg->setInvokeID(getNewInvokeId());

    msg->setDstAddr(obj->getDstAddress());

    signalizeSendData(msg);
}

void RIBd::sendStartOperationRequest(OperationObj* obj) {
    Enter_Method("sendStartOperationRequest()");
}

void RIBd::sendStartOperationResponse(OperationObj* obj) {
    Enter_Method("sendStartOperationResponse()");
}

void RIBd::processMConnect(CDAPMessage* msg) {
    CDAP_M_Connect* msg1 = check_and_cast<CDAP_M_Connect*>(msg);

    EV << "Received M_Connect";

    if (msg1) {
        signalizeConnectRequest(msg);
    }
}

void RIBd::processMConnectR(CDAPMessage* msg) {
    CDAP_M_Connect_R* msg1 = check_and_cast<CDAP_M_Connect_R*>(msg);

    EV << "Received M_Connect_R";

    if (msg1) {
        if (!msg1->getResult().resultValue) {
            signalizeConnectResponsePositive(msg);
        }
        else {
            signalizeConnectResponseNegative(msg);
        }
    }
}

void RIBd::processMStart(CDAPMessage* msg) {
    CDAP_M_Start* msg1 = check_and_cast<CDAP_M_Start*>(msg);

    EV << "Received M_Start";
    object_t object = msg1->getObject();
    EV << " with object '" << object.objectClass << "'" << endl;

    //CongestionNotification CongestDescr
    if (dynamic_cast<CongestionDescriptor*>(object.objectVal)) {
        CongestionDescriptor* congdesc = (check_and_cast<CongestionDescriptor*>(object.objectVal))->dup();
        congdesc->getConnectionId().swapCepIds();
        EV << "\n===========\n" << congdesc->getConnectionId().info();
        signalizeCongestionNotification(congdesc);
    }

    //Enrollment
    if (dynamic_cast<EnrollmentObj*>(object.objectVal)) {
       signalizeStartEnrollmentRequest(msg);
    }

}

void RIBd::processMStartR(CDAPMessage* msg) {
    CDAP_M_Start_R* msg1 = check_and_cast<CDAP_M_Start_R*>(msg);

    EV << "Received M_Start_R";
    object_t object = msg1->getObject();
    EV << " with object '" << object.objectClass << "'" << endl;

    //Enrollment
    if (dynamic_cast<EnrollmentObj*>(object.objectVal)) {
        signalizeStartEnrollmentResponse(msg);
    }
}

void RIBd::processMStop(CDAPMessage* msg) {
    CDAP_M_Stop* msg1 = check_and_cast<CDAP_M_Stop*>(msg);

    EV << "Received M_Stop";
    object_t object = msg1->getObject();
    EV << " with object '" << object.objectClass << "'" << endl;

    //Enrollment
    if (dynamic_cast<EnrollmentObj*>(object.objectVal)) {
        signalizeStopEnrollmentRequest(msg);
    }
}

void RIBd::processMStopR(CDAPMessage* msg) {
    CDAP_M_Stop_R* msg1 = check_and_cast<CDAP_M_Stop_R*>(msg);

    EV << "Received M_Stop_R";
    object_t object = msg1->getObject();
    EV << " with object '" << object.objectClass << "'" << endl;

    //Enrollment
    if (dynamic_cast<EnrollmentObj*>(object.objectVal)) {
        signalizeStopEnrollmentResponse(msg);
    }
}

void RIBd::sendCACE(CDAPMessage* msg) {
    Enter_Method("sendCACE()");

    //TODO: add invoke id

    signalizeSendCACE(msg);
}
