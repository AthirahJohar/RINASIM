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

#include "FAI.h"

const char*     TIM_CREREQ          = "CreateRequestTimer";
const char*     MOD_ALLOCRETRYPOLICY= "allocateRetryPolicy";

Define_Module(FAI);

FAI::FAI() : FAIBase() {
    FaModule = NULL;
    //creReqTimer = NULL;
}

FAI::~FAI() {
    FaModule = NULL;
    FlowObject = NULL;
    degenerateDataTransfer = false;
    localPortId     = VAL_UNDEF_PORTID;
    localCEPId      = VAL_UNDEF_CEPID;
    remotePortId    = VAL_UNDEF_PORTID;
    remoteCEPId     = VAL_UNDEF_CEPID;
    //if (creReqTimer)
    //    cancelAndDelete(creReqTimer);
}

void FAI::initialize() {
    localPortId  = par(PAR_LOCALPORTID);
    localCEPId   = par(PAR_LOCALCEPID);
    remotePortId = par(PAR_REMOTEPORTID);
    remoteCEPId  = par(PAR_REMOTECEPID);

    //creReqTimeout = par(PAR_CREREQTIMEOUT).doubleValue();

    AllocRetryPolicy = check_and_cast<AllocateRetryBase*>(getParentModule()->getSubmodule(MOD_ALLOCRETRYPOLICY));

    WATCH(degenerateDataTransfer);
    WATCH_PTR(FlowObject);

    initSignalsAndListeners();
}

void FAI::postInitialize(FABase* fa, Flow* fl, EFCP* efcp) {
    //Initialize pointers! It cannot be done during model creation :(
    this->FaModule = fa;
    this->FlowObject = fl;
    this->EfcpModule = efcp;
}

bool FAI::receiveAllocateRequest() {
    Enter_Method("receiveAllocateRequest()");

    //Check for proper FSM state
    FAITable* ft = FaModule->getFaiTable();
    FAITableEntry* fte = ft->findEntryByFlow(FlowObject);
    if (fte->getAllocateStatus() != FAITableEntry::ALLOC_PEND) {
        EV << "Cannot allocate flow which is not in pending state" << endl;
        return false;
    }

    //Invoke NewFlowReqPolicy
    bool status = this->FaModule->invokeNewFlowRequestPolicy(this->FlowObject);
    if (!status){
        EV << "invokeNewFlowPolicy() failed"  << endl;
        ft->changeAllocStatus(FlowObject, FAITableEntry::ALLOC_NEGA);
        this->signalizeAllocateResponseNegative();
        return false;
    }

    status = this->createEFCPI();
    if (!status) {
        EV << "createEFCP() failed" << endl;
        ft->changeAllocStatus(FlowObject, FAITableEntry::ALLOC_NEGA);
        this->signalizeAllocateResponseNegative();
        return false;
    }

    status = this->createBindings();
    if (!status) {
        EV << "createBindings() failed" << endl;
        ft->changeAllocStatus(FlowObject, FAITableEntry::ALLOC_NEGA);
        this->signalizeAllocateResponseNegative();
        return false;
    }

    //EV << "!!!!!!" << FlowObject->info() << endl << FlowObject->getDstNeighbor() << endl;

    // bind this flow to a suitable (N-1)-flow
    RABase* raModule = (RABase*) getModuleByPath("^.^.resourceAllocator.ra");

    status = isDegenerateDataTransfer() ? true : raModule->bindNFlowToNM1Flow(FlowObject);
    //IF flow is already available then schedule M_Create(Flow)
    if (status) {
        this->signalizeCreateFlowRequest();
    }

    //Everything went fine
    return true;
}

bool FAI::receiveAllocateResponsePositive() {
    Enter_Method("receiveAllocateResponsePositive()");

    //Check for proper FSM state
    FAITable* ft = FaModule->getFaiTable();
    FAITableEntry* fte = ft->findEntryByFlow(FlowObject);
    if (fte->getAllocateStatus() != FAITableEntry::ALLOC_PEND) {
        EV << "Cannot continue allocation of flow which is not in pending state" << endl;
        return false;
    }


    //Instantiate EFCPi
    bool status = this->createEFCPI();
    if (!status) {
        EV << "createEFCP() failed" << endl;
        ft->changeAllocStatus(FlowObject, FAITableEntry::ALLOC_NEGA);
        //Schedule negative M_Create_R(Flow)
        this->signalizeCreateFlowResponseNegative();
        return false;
    }

    //Interconnect IPC <-> EFCPi <-> RMT
    status = this->createBindings();
    if (!status) {
        EV << "createBindings() failed" << endl;
        ft->changeAllocStatus(FlowObject, FAITableEntry::ALLOC_NEGA);
        //Schedule M_Create_R(Flow-)
        this->signalizeCreateFlowResponseNegative();
        return false;
    }

    // bind this flow to a suitable (N-1)-flow
    RABase* raModule = (RABase*) getModuleByPath("^.^.resourceAllocator.ra");
    status = isDegenerateDataTransfer() ? true : raModule->bindNFlowToNM1Flow(FlowObject);

    ft->changeAllocStatus(FlowObject, FAITableEntry::TRANSFER);
    //Signalizes M_Create_R(flow)
    if (status) {
        this->signalizeCreateFlowResponsePositive();
    }

    return true;
}

void FAI::receiveAllocateResponseNegative() {
    Enter_Method("receiveAllocateResponseNegative()");
    //Check for proper FSM state
    FAITable* ft = FaModule->getFaiTable();
    FAITableEntry* fte = ft->findEntryByFlow(FlowObject);
    if (fte->getAllocateStatus() != FAITableEntry::ALLOC_PEND) {
        EV << "Cannot continue allocation of flow which is not in pending state" << endl;
        return;
    }

    ft->changeAllocStatus(FlowObject, FAITableEntry::ALLOC_NEGA);

    //IF it is not DDT then retry M_CREATE
    //if (!isDegenerateDataTransfer())
    this->signalizeCreateFlowResponseNegative();
}

bool FAI::receiveCreateRequest() {
    Enter_Method("receiveCreateRequest()");

    //Check for proper FSM state
    FAITable* ft = FaModule->getFaiTable();
    FAITableEntry* fte = ft->findEntryByFlow(FlowObject);
    if (fte->getAllocateStatus() != FAITableEntry::ALLOC_PEND) {
        EV << "Cannot allocate flow which is not in pending state" << endl;
        return false;
    }

    //Invoke NewFlowReqPolicy
    bool status = this->FaModule->invokeNewFlowRequestPolicy(this->FlowObject);
    if (!status){
        EV << "invokeNewFlowPolicy() failed"  << endl;
        //Schedule negative M_Create_R(Flow)
        ft->changeAllocStatus(FlowObject, FAITableEntry::ALLOC_NEGA);
        this->signalizeCreateFlowResponseNegative();
        return false;
    }

    //Create IPC north gates
    createNorthGates();

    //Pass AllocationRequest to AP or RIBd
    this->signalizeAllocationRequestFromFai();

    //Everything went fine
    return true;
}

bool FAI::receiveDeallocateRequest() {
    Enter_Method("receiveDeallocateRequest()");

    //Check for proper FSM state
    FAITable* ft = FaModule->getFaiTable();
    FAITableEntry* fte = ft->findEntryByFlow(FlowObject);
    if (fte->getAllocateStatus() != FAITableEntry::DEALLOC_PEND) {
        EV << "Cannot deallocate flow which is not in deallocate pending state" << endl;
        return false;
    }

    //deleteBindings
    bool status = this->deleteBindings();

    //Signalize M_Delete(Flow)
    this->signalizeDeleteFlowRequest();

    return status;
}

void FAI::receiveDeleteRequest(Flow* flow) {
    Enter_Method("receiveDeleteRequest()");

    //Check for proper FSM state
    FAITable* ft = FaModule->getFaiTable();
    FAITableEntry* fte = ft->findEntryByFlow(FlowObject);
    if (fte->getAllocateStatus() != FAITableEntry::TRANSFER) {
        EV << "Cannot deallocate flow which is not in transfer state" << endl;
        return;
    }

    ft->changeAllocStatus(FlowObject, FAITableEntry::DEALLOC_PEND);

    //Get deallocation invokeId from Request
    FlowObject->setDeallocInvokeId(flow->getDeallocInvokeId());

    //Notify application
    signalizeDeallocateRequestFromFai();

    //DeleteBindings
    this->deleteBindings();

    //Signalizes M_Delete_R(Flow)
    this->signalizeDeleteFlowResponse();

    ft->changeAllocStatus(FlowObject, FAITableEntry::DEALLOCATED);
    fte->setTimeDeleted(simTime());
}

bool FAI::receiveCreateResponseNegative() {
    Enter_Method("receiveCreResNegative()");

    //invokeAllocateRetryPolicy
    bool status = this->invokeAllocateRetryPolicy();

    //If number of retries IS NOT reached THEN ...
    if (status) {
        signalizeCreateFlowRequest();
    }
    //...otherwise signalize to AE or RIBd failure
    else  {
        EV << "invokeAllocateRetryPolicy() failed"  << endl;
        FaModule->getFaiTable()->changeAllocStatus(FlowObject, FAITableEntry::ALLOC_NEGA);
        this->signalizeAllocateResponseNegative();
    }

    return status;
}

bool FAI::receiveCreateResponsePositive(Flow* flow) {
    Enter_Method("receiveCreResPositive()");
    //TODO: Vesely - D-Base-2011-015.pdf, p.9
    //              Create bindings. WTF? Bindings should be already created!''

    //cancelEvent(creReqTimer);

    //Change dstCep-Id and dstPortId according to new information
    FlowObject->getConnectionId().setDstCepId(flow->getConId().getDstCepId());
    FlowObject->setDstPortId(flow->getDstPortId());
    remotePortId = flow->getDstPortId();
    remoteCEPId = flow->getConId().getDstCepId();
    par(PAR_REMOTEPORTID) = remotePortId;
    par(PAR_REMOTECEPID) = remoteCEPId;

    //Change status
    FaModule->getFaiTable()->changeAllocStatus(FlowObject, FAITableEntry::TRANSFER);

    if (FlowObject->isManagementFlowLocalToIPCP()) {
        signalizeAllocateRequestToOtherFais( FlowObject );
    }
    else {
        //Pass Allocate Response to AE or RIBd
        this->signalizeAllocateResponsePositive();
    }

    //FIXME: Vesely - always true
    return true;
}

void FAI::receiveDeleteResponse() {
    Enter_Method("receiveDeleteResponse()");

    //Check for proper FSM state
    FAITable* ft = FaModule->getFaiTable();
    FAITableEntry* fte = ft->findEntryByFlow(FlowObject);
    if (fte->getAllocateStatus() != FAITableEntry::DEALLOC_PEND) {
        EV << "Cannot deallocate flow which is not in deallocatre pending state" << endl;
        return;
    }

    //Notify application
    signalizeDeallocateRequestFromFai();

    ft->changeAllocStatus(FlowObject, FAITableEntry::DEALLOCATED);
    fte->setTimeDeleted(simTime());
}

void FAI::handleMessage(cMessage *msg) {
    /*
    //CreateRequest was not delivered in time
    if ( !strcmp(msg->getName(), TIM_CREREQ) ) {
        //Increment and resend
        bool status = receiveCreateResponseNegative();
        if (!status)
            EV << "CreateRequest retries reached its maximum!" << endl;
    }
    */
}

std::string FAI::info() const {
    std::stringstream os;
    os << "FAI>" << endl
       << "\tlocal  Port-ID: " << this->localPortId << "\tCEP-ID: " << this->localCEPId << endl
       << "\tremote Port-ID: " << this->remotePortId << "\tCEP-ID: " << this->remoteCEPId;
    return os.str();
}

//Free function
std::ostream& operator<< (std::ostream& os, const FAI& fai) {
    return os << fai.info();
}

bool FAI::createEFCPI() {
    EV << this->getFullPath() << " attempts to create EFCP instance" << endl;
    //Create EFCPI for local bindings
    EFCPInstance* efcpi = EfcpModule->createEFCPI(FlowObject, localCEPId, localPortId);
    return efcpi ? true : false;
}

bool FAI::createBindings() {
    EV << this->getFullPath() << " attempts to bind EFCP and RMT" << endl;

    cModule* IPCModule = FaModule->getParentModule()->getParentModule();

    std::ostringstream nameEfcpNorth;
    nameEfcpNorth << GATE_APPIO_ << localPortId;
    cModule* efcpModule = IPCModule->getModuleByPath(".efcp");
    cGate* gateEfcpUpIn = efcpModule->gateHalf(nameEfcpNorth.str().c_str(), cGate::INPUT);
    cGate* gateEfcpUpOut = efcpModule->gateHalf(nameEfcpNorth.str().c_str(), cGate::OUTPUT);

    //Management Flow should be connected with RIBd
    if (FlowObject->isManagementFlowLocalToIPCP()) {
        std::ostringstream ribdName;
        ribdName << GATE_EFCPIO_ << localPortId;
        cModule* ribdModule = IPCModule->getModuleByPath(".ribDaemon");
        cModule* ribdSplitterModule = IPCModule->getModuleByPath(".ribDaemon.ribdSplitter");

        if (!ribdModule->hasGate(ribdName.str().c_str()))
            {createNorthGates();}

        cGate* gateRibdIn  = ribdModule->gateHalf(ribdName.str().c_str(), cGate::INPUT);
        cGate* gateRibdOut = ribdModule->gateHalf(ribdName.str().c_str(), cGate::OUTPUT);
        cGate* gateRibdSplitIn  = ribdSplitterModule->gateHalf(ribdName.str().c_str(), cGate::INPUT);
        cGate* gateRibdSplitOut = ribdSplitterModule->gateHalf(ribdName.str().c_str(), cGate::OUTPUT);

        //EFCPi.efcpio <--> RIBDaemon.efcpIo_ <--> RIBDaemon.ribdSplitter.efcpIo_
        gateEfcpUpOut->connectTo(gateRibdIn);
        gateRibdIn->connectTo(gateRibdSplitIn);
        gateRibdSplitOut->connectTo(gateRibdOut);
        gateRibdOut->connectTo(gateEfcpUpIn);
    }
    //Data flow
    else {
        std::ostringstream nameIpcDown;
        nameIpcDown << GATE_NORTHIO_ << localPortId;

        //IF called as consequence of AllocateRequest then createNorthGate
        //ELSE (called as consequence of CreateRequestFlow) skip
        if (!IPCModule->hasGate(nameIpcDown.str().c_str()))
            {createNorthGates();}

        cGate* gateIpcDownIn = IPCModule->gateHalf(nameIpcDown.str().c_str(), cGate::INPUT);
        cGate* gateIpcDownOut = IPCModule->gateHalf(nameIpcDown.str().c_str(), cGate::OUTPUT);

        //IPCModule.northIo <--> Efcp.fai
        gateEfcpUpOut->connectTo(gateIpcDownOut);
        gateIpcDownIn->connectTo(gateEfcpUpIn);
    }

    //Create bindings in RMT
    RMT* rmtModule = (RMT*) IPCModule->getModuleByPath(".relayAndMux.rmt");
    rmtModule->createEfcpiGate(localCEPId);

    std::ostringstream nameRmtUp;
    nameRmtUp << GATE_EFCPIO_ << localCEPId;
    cGate* gateRmtUpIn = rmtModule->getParentModule()->gateHalf(nameRmtUp.str().c_str(), cGate::INPUT);
    cGate* gateRmtUpOut = rmtModule->getParentModule()->gateHalf(nameRmtUp.str().c_str(), cGate::OUTPUT);

    std::ostringstream nameEfcpDown;
    nameEfcpDown << GATE_RMT_ << localCEPId;
    cGate* gateEfcpDownIn = efcpModule->gateHalf(nameEfcpDown.str().c_str(), cGate::INPUT);
    cGate* gateEfcpDownOut = efcpModule->gateHalf(nameEfcpDown.str().c_str(), cGate::OUTPUT);

    //Efcp.rmt <--> Rmt.efcpIo
    gateRmtUpOut->connectTo(gateEfcpDownIn);
    gateEfcpDownOut->connectTo(gateRmtUpIn);

    return gateEfcpDownIn->isConnected() && gateEfcpDownOut->isConnected()
           && gateEfcpUpIn->isConnected() && gateEfcpUpOut->isConnected()
           && gateRmtUpIn->isConnected() && gateRmtUpOut->isConnected();

}

bool FAI::deleteBindings() {
    EV << this->getFullPath() << " attempts to disconnect bindings between EFCP, IPC and RMT" << endl;

    //Flush All messages in EFCPI
    EfcpModule->deleteEFCPI(this->getFlow());

    //Management flow
    if (FlowObject->isManagementFlowLocalToIPCP()) {
        std::ostringstream ribdName;
        ribdName << GATE_EFCPIO_ << localPortId;
        cModule* ribdModule = this->getModuleByPath("^.^.ribDaemon");
        cModule* ribdSplitterModule = this->getModuleByPath("^.^.ribDaemon.ribdSplitter");

        cGate* gateRibdIn  = ribdModule->gateHalf(ribdName.str().c_str(), cGate::INPUT);
        cGate* gateRibdOut = ribdModule->gateHalf(ribdName.str().c_str(), cGate::OUTPUT);
        cGate* gateRibdSplitIn  = ribdSplitterModule->gateHalf(ribdName.str().c_str(), cGate::INPUT);
        cGate* gateRibdSplitOut = ribdSplitterModule->gateHalf(ribdName.str().c_str(), cGate::OUTPUT);

        gateRibdIn->disconnect();
        gateRibdOut->disconnect();
        gateRibdSplitIn->disconnect();
        gateRibdSplitOut->disconnect();
    }
    //Data flow
    else {
        cModule* IPCModule = FaModule->getParentModule()->getParentModule();
        std::ostringstream nameIpcDown;
        nameIpcDown << GATE_NORTHIO_ << localPortId;
        cGate* gateIpcDownIn = IPCModule->gateHalf(nameIpcDown.str().c_str(), cGate::INPUT);
        cGate* gateIpcDownOut = IPCModule->gateHalf(nameIpcDown.str().c_str(), cGate::OUTPUT);

        //IPCModule.northIo
        gateIpcDownOut->disconnect();
        gateIpcDownIn->disconnect();
    }

    //Delete EFCP bindings
    std::ostringstream nameEfcpNorth;
    nameEfcpNorth << GATE_APPIO_ << localPortId;
    cModule* efcpModule = this->getModuleByPath("^.^.efcp");
    cGate* gateEfcpUpIn = efcpModule->gateHalf(nameEfcpNorth.str().c_str(), cGate::INPUT);
    cGate* gateEfcpUpOut = efcpModule->gateHalf(nameEfcpNorth.str().c_str(), cGate::OUTPUT);
    gateEfcpUpIn->disconnect();
    gateEfcpUpOut->disconnect();

    //Delete bindings in RMT
    RMT* rmtModule = (RMT*) this->getModuleByPath("^.^.relayAndMux.rmt");

    std::ostringstream nameRmtUp;
    nameRmtUp << GATE_EFCPIO_ << localCEPId;
    cGate* gateRmtUpIn = rmtModule->getParentModule()->gateHalf(nameRmtUp.str().c_str(), cGate::INPUT);
    cGate* gateRmtUpOut = rmtModule->getParentModule()->gateHalf(nameRmtUp.str().c_str(), cGate::OUTPUT);

    std::ostringstream nameEfcpDown;
    nameEfcpDown << GATE_RMT_ << localCEPId;
    cGate* gateEfcpDownIn = efcpModule->gateHalf(nameEfcpDown.str().c_str(), cGate::INPUT);
    cGate* gateEfcpDownOut = efcpModule->gateHalf(nameEfcpDown.str().c_str(), cGate::OUTPUT);

    //Efcp.rmt <- XX -> Rmt.efcpIo
    gateRmtUpOut->disconnect();
    gateRmtUpIn->disconnect();
    gateEfcpDownIn->disconnect();
    gateEfcpDownOut->disconnect();

    rmtModule->deleteEfcpiGate(localCEPId);

    return true;
}

bool FAI::invokeAllocateRetryPolicy() {
    //Increase CreateFlowRetries
    //cancelEvent(creReqTimer);
    return AllocRetryPolicy->run(*getFlow());
}

void FAI::initSignalsAndListeners() {
    cModule* catcher2 = this->getParentModule()->getParentModule();
    cModule* catcher3 = this->getParentModule()->getParentModule()->getParentModule();
    //Signals that module emits
    sigFAIAllocReq      = registerSignal(SIG_FAI_AllocateRequest);
    sigFAIDeallocReq    = registerSignal(SIG_FAI_DeallocateRequest);
    sigFAIDeallocRes    = registerSignal(SIG_FAI_DeallocateResponse);
    sigFAIAllocResPosi  = registerSignal(SIG_FAI_AllocateResponsePositive);
    sigFAIAllocResNega  = registerSignal(SIG_FAI_AllocateResponseNegative);
    sigFAICreReq        = registerSignal(SIG_FAI_CreateFlowRequest);
    sigFAIDelReq        = registerSignal(SIG_FAI_DeleteFlowRequest);
    sigFAIDelRes        = registerSignal(SIG_FAI_DeleteFlowResponse);
    sigFAICreResNega    = registerSignal(SIG_FAI_CreateFlowResponseNegative);
    sigFAICreResPosi    = registerSignal(SIG_FAI_CreateFlowResponsePositive);
    sigFAIAllocFinMgmt   = registerSignal(SIG_FAI_AllocateFinishManagement);

    //Signals that module processes
    //  AllocationRequest
    this->lisAllocReq       = new LisFAIAllocReq(this);
    catcher3->subscribe(SIG_toFAI_AllocateRequest, this->lisAllocReq);
    //  AllocationRespNegative
    this->lisAllocResNega   = new LisFAIAllocResNega(this);
    catcher3->subscribe(SIG_AERIBD_AllocateResponseNegative, this->lisAllocResNega);
    //  AllocationRespPositive
    this->lisAllocResPosi   = new LisFAIAllocResPosi(this);
    catcher3->subscribe(SIG_AERIBD_AllocateResponsePositive, this->lisAllocResPosi);
//    //  CreateFlowRequest
//    this->lisCreReq         = new LisFAICreReq(this);
//    catcher->subscribe(SIG_FAI_CreateFlowRequest, this->lisCreReq);
    //  CreateFlowResponseNegative
    this->lisCreResNega     = new LisFAICreResNega(this);
    catcher3->subscribe(SIG_RIBD_CreateFlowResponseNegative, this->lisCreResNega);
    // CreateFlowResponsePositive
    this->lisCreResPosi     = new LisFAICreResPosi(this);
    catcher3->subscribe(SIG_RIBD_CreateFlowResponsePositive, this->lisCreResPosi);

    //  CreateFlowResponseNegative
    lisCreResNegaNmO     = new LisFAICreResNegaNminusOne(this);
    catcher3->subscribe(SIG_RIBD_CreateFlowResponseNegative, lisCreResNegaNmO);
    // CreateFlowResponsePositive
    lisCreResPosiNmO     = new LisFAICreResPosiNminusOne(this);
    catcher3->subscribe(SIG_RIBD_CreateFlowResponsePositive, lisCreResPosiNmO);

    //DeleteRequestFlow
    lisDelReq = new LisFAIDelReq(this);
    catcher2->subscribe(SIG_RIBD_DeleteRequestFlow, lisDelReq);

    //DeleteResponseFlow
    lisDelRes = new LisFAIDelRes(this);
    catcher2->subscribe(SIG_RIBD_DeleteResponseFlow, lisDelRes);

}

void FAI::signalizeCreateFlowRequest() {
    //creReqTimer = new cMessage(TIM_CREREQ);
    //Start timer
    //scheduleAt(simTime() + creReqTimeout, creReqTimer);
    //Signalize RIBd to send M_CREATE(flow)
    emit(this->sigFAICreReq, FlowObject);
}

void FAI::signalizeDeleteFlowResponse() {
    emit(this->sigFAIDelRes, this->FlowObject);

}

void FAI::signalizeCreateFlowResponsePositive() {
    emit(this->sigFAICreResPosi, FlowObject);
}

void FAI::signalizeCreateFlowResponseNegative() {
    emit(this->sigFAICreResNega, FlowObject);
}

void FAI::signalizeAllocationRequestFromFai() {
    emit(sigFAIAllocReq, FlowObject);
}

void FAI::signalizeDeleteFlowRequest() {
    emit(this->sigFAIDelReq, this->FlowObject);
}

void FAI::signalizeAllocateResponseNegative() {
    emit(this->sigFAIAllocResNega, this->FlowObject);
}

void FAI::signalizeDeallocateRequestFromFai() {
    emit(this->sigFAIDeallocReq, this->FlowObject);
}

void FAI::signalizeDeallocateResponseFromFai() {
    emit(this->sigFAIDeallocRes, this->FlowObject);
}

int FAI::getLocalCepId() const {
    return localCEPId;
}

void FAI::setLocalCepId(int localCepId) {
    localCEPId = localCepId;
}

int FAI::getLocalPortId() const {
    return localPortId;
}

void FAI::setLocalPortId(int localPortId) {
    this->localPortId = localPortId;
}

int FAI::getRemoteCepId() const {
    return remoteCEPId;
}

void FAI::setRemoteCepId(int remoteCepId) {
    remoteCEPId = remoteCepId;
}

int FAI::getRemotePortId() const {
    return remotePortId;
}

void FAI::setRemotePortId(int remotePortId) {
    this->remotePortId = remotePortId;
}

void FAI::signalizeAllocateResponsePositive() {
    emit(this->sigFAIAllocResPosi, this->FlowObject);
}

void FAI::createNorthGates() {
    //Management flow
    if (FlowObject->isManagementFlowLocalToIPCP()) {
        std::ostringstream ribdName;
        ribdName << GATE_EFCPIO_ << localPortId;
        cModule* ribdModule = this->getModuleByPath("^.^.ribDaemon");
        cModule* ribdSplitterModule = this->getModuleByPath("^.^.ribDaemon.ribdSplitter");
        ribdModule->addGate(ribdName.str().c_str(), cGate::INOUT, false);
        ribdSplitterModule->addGate(ribdName.str().c_str(), cGate::INOUT, false);
    }
    //Data flow
    else {
        std::ostringstream nameIpcDown;
        nameIpcDown << GATE_NORTHIO_ << localPortId;
        cModule* IPCModule = FaModule->getParentModule()->getParentModule();
        IPCModule->addGate(nameIpcDown.str().c_str(), cGate::INOUT, false);
    }
    return;
}

void FAI::receiveCreateFlowResponsePositiveFromNminusOne() {
    Enter_Method("receiveCreFlowResPositiveFromNminusOne()");
    //Schedule M_Create(Flow)
    this->signalizeCreateFlowRequest();
}

void FAI::receiveCreateFlowResponseNegativeFromNminusOne() {
    this->signalizeAllocateResponseNegative();
}

void FAI::signalizeAllocateRequestToOtherFais(Flow* flow) {
    emit(sigFAIAllocFinMgmt, flow);
}
