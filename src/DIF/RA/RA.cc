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
 * @file RA.cc
 * @author Tomas Hykel (xhykel01@stud.fit.vutbr.cz)
 * @brief Monitoring and adjustment of IPC process operations
 * @detail
 */

#include "RA.h"

Define_Module(RA);

// QoS loader parameters
const char* PAR_QOSDATA              = "qoscubesData";
const char* ELEM_QOSCUBE             = "QoSCube";
const char* PAR_QOSREQ               = "qosReqData";
const char* ELEM_QOSREQ              = "QoSReq";
const char* ATTR_ID                  = "id";


void RA::initialize(int stage)
{
    if (stage == 1)
    {
        // determine and set RMT mode of operation
        setRMTMode();
        // preallocate flows
        initFlowAlloc();
        return;
    }

    // retrieve pointers to other modules
    thisIPC = this->getParentModule()->getParentModule();
    rmtModule = thisIPC->getSubmodule("relayAndMux");

    // Get access to the forwarding and routing functionalities...
    fwdtg = check_and_cast<IntPDUFG *>
        (getModuleByPath("^.pduFwdGenerator"));


    difAllocator = check_and_cast<DA*>
        (getModuleByPath("^.^.^.difAllocator.da"));
    flowTable = check_and_cast<NM1FlowTable*>
        (getModuleByPath("^.nm1FlowTable"));
    rmt = check_and_cast<RMT*>
        (getModuleByPath("^.^.relayAndMux.rmt"));
    rmtAllocator = check_and_cast<RMTModuleAllocator*>
        (getModuleByPath("^.^.relayAndMux.allocator"));
    fa = check_and_cast<FABase*>
            (getModuleByPath("^.^.flowAllocator.fa"));


    // retrieve pointers to policies
    qAllocPolicy = check_and_cast<QueueAllocBase*>
        (getModuleByPath("^.queueAllocPolicy"));

    // initialize attributes
    std::ostringstream os;
    os << thisIPC->par(PAR_IPCADDR).stringValue() << "_"
       << thisIPC->par(PAR_DIFNAME).stringValue();
    processName  = os.str();

    initSignalsAndListeners();
    initQoSCubes();

    WATCH_LIST(this->QoSCubes);
}

void RA::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {

        if (!opp_strcmp(msg->getName(), "RA-CreateConnections"))
        {
            std::list<Flow*>* flows = preparedFlows[simTime()];

            while (!flows->empty())
            {
                // Data connections are carried by (N-1)-flows alone, whereas
                // management connections use (N-1)-management flows ALONG WITH
                // (N)-management flows. Thus, data flows are allocated via
                // (N-1)-FA and management flows via (N)-FA.
                // In addition to that, we can assume that (N-1)-data flows
                // are going to require (N)-management, so it's allocated as well.


                Flow* flow = flows->front();
                if (flow->isManagementFlow())
                { // mgmt flow
                    createNFlow(flow);
                }
                else
                { // data flow
                    createNM1Flow(flow);
                }

                flows->pop_front();
            }

            delete flows;
            delete msg;
        }
    }
}

void RA::initSignalsAndListeners()
{
    sigRACreFloPosi = registerSignal(SIG_RA_CreateFlowPositive);
    sigRACreFloNega = registerSignal(SIG_RA_CreateFlowNegative);
    sigRASDReqFromRMT = registerSignal(SIG_RA_InvokeSlowdown);
    sigRASDReqFromRIB = registerSignal(SIG_RA_ExecuteSlowdown);
    sigRAMgmtAllocd = registerSignal(SIG_RA_MgmtFlowAllocated);
    sigRAMgmtDeallocd = registerSignal(SIG_RA_MgmtFlowDeallocated);

    lisRAAllocResPos = new LisRAAllocResPos(this);
    thisIPC->subscribe(SIG_FAI_AllocateResponsePositive, lisRAAllocResPos);

    lisRACreAllocResPos = new LisRACreAllocResPos(this);
    thisIPC->subscribe(SIG_FAI_CreateFlowResponsePositive, lisRACreAllocResPos);

    lisRACreFlow = new LisRACreFlow(this);
    thisIPC->subscribe(SIG_RIBD_CreateFlow, lisRACreFlow);

    lisRACreResPosi = new LisRACreResPosi(this);
    thisIPC->getParentModule()->
            subscribe(SIG_RIBD_CreateFlowResponsePositive, this->lisRACreResPosi);

    lisEFCPStopSending = new LisEFCPStopSending(this);
    thisIPC->getParentModule()->
            subscribe(SIG_EFCP_StopSending, this->lisEFCPStopSending);

    lisEFCPStartSending = new LisEFCPStartSending(this);
    thisIPC->getParentModule()->
            subscribe(SIG_EFCP_StartSending, this->lisEFCPStartSending);

    lisRMTSDReq = new LisRMTSlowdownRequest(this);
    thisIPC->subscribe(SIG_RMT_SlowdownRequest, this->lisRMTSDReq);

    lisRIBCongNotif = new LisRIBCongNotif(this);
    thisIPC->subscribe(SIG_RIBD_CongestionNotification, this->lisRIBCongNotif);
}

void RA::initFlowAlloc()
{
    cXMLElement* dirXml = par("preallocation").xmlValue();
    cXMLElementList timeMap = dirXml->getChildrenByTagName("SimTime");

    for (cXMLElementList::const_iterator it = timeMap.begin(); it != timeMap.end(); ++it)
    {
        cXMLElement* m = *it;
        simtime_t time = static_cast<simtime_t>(
                atoi(m->getAttribute("t")));


        cXMLElementList connMap = m->getChildrenByTagName("Connection");
        for (cXMLElementList::const_iterator jt = connMap.begin(); jt != connMap.end(); ++jt)
        {

            cXMLElement* n = *jt;
            const char* src = n->getAttribute("src");
            if (opp_strcmp(src, processName.c_str()))
            {
                continue;
            }


            const char* dst = n->getAttribute("dst");
            const char* qosReqID_s = n->getAttribute("qosReq");
            APNamingInfo srcAPN = APNamingInfo(APN(src));
            APNamingInfo dstAPN = APNamingInfo(APN(dst));

            QoSReq* qosReq = NULL;

            if (!opp_strcmp(qosReqID_s, "mgmt"))
            {
                qosReq = &mgmtReqs;
            }
            else
            {
                qosReq = initQoSReqById(static_cast<unsigned short>
                                        (atoi(qosReqID_s)));
            }


            if (qosReq == NULL) continue;

            Flow *flow = new Flow(srcAPN, dstAPN);
            flow->setQosRequirements(*qosReq);

            if (preparedFlows[time] == NULL)
            {
                preparedFlows[time] = new std::list<Flow*>;
                cMessage* msg = new cMessage("RA-CreateConnections");
                scheduleAt(simTime() + time, msg);

            }

            preparedFlows[time]->push_back(flow);
        }
    }
}

/**
 * Sets up RMT's mode of operation by "recursion level" of this IPC process
 */
void RA::setRMTMode()
{
    // identify the role of this IPC process in processing system
    std::string bottomGate = thisIPC->gate("southIo$o", 0)->getNextGate()->getName();

    if (bottomGate == "medium$o" || bottomGate == "mediumIntra$o" || bottomGate == "mediumInter$o")
    {
        // we're on wire! this is the bottommost "interface" DIF
        rmt->setOnWire(true);
        // connect RMT to the medium
        bindMediumToRMT();
    }
        else if (bottomGate == "northIo$i")
    { // other IPC processes are below us
        rmt->setOnWire(false);
    }

    if (thisIPC->par("relay").boolValue() == true)
    { // this is an IPC process that uses PDU forwarding
        rmt->enableRelay();
    }
}

/**
 * Initializes QoS cubes from given XML configuration directive.
 */
void RA::initQoSCubes()
{
    cXMLElement* qosXml = NULL;
    if (par(PAR_QOSDATA).xmlValue() != NULL
            && par(PAR_QOSDATA).xmlValue()->hasChildren())
        qosXml = par(PAR_QOSDATA).xmlValue();
    else
        error("qoscubesData parameter not initialized!");

    // load cubes from XML
    cXMLElementList cubes = qosXml->getChildrenByTagName(ELEM_QOSCUBE);
    for (cXMLElementList::iterator it = cubes.begin(); it != cubes.end(); ++it)
    {
        cXMLElement* m = *it;
        if (!m->getAttribute(ATTR_ID))
        {
            EV << "Error parsing QoSCube. Its ID is missing!" << endl;
            continue;
        }

        cXMLElementList attrs = m->getChildren();
        QoSCube cube = QoSCube(attrs);
        cube.setQosId(m->getAttribute(ATTR_ID));

        //Integrity check!!!
        if (cube.isDefined())
        {
            QoSCubes.push_back(cube);
        }
        else
        {
            EV << "QoSCube with ID " << cube.getQosId()
                    << " contains DO-NOT-CARE parameter. It is not fully defined,"
                    << " thus it is not loaded into RA's QoS-cube set!"
                    << endl;
        }
    }

    if (!QoSCubes.size())
    {
        std::ostringstream os;
        os << this->getFullPath()
                << " does not have any QoSCube in its set. "
                << "It cannot work without at least one valid QoS cube!"
                << endl;
        error(os.str().c_str());
    }

    // add a static QoS cube for management
    QoSCubes.push_back(QoSCube::MANAGEMENT);

    // add a QoS requirements object
    mgmtReqs = QoSReq::MANAGEMENT;
}

/**
 * Initializes QoS requirement identified by give id
 * @param id ID of the QoSReq to be initialized
 */
QoSReq* RA::initQoSReqById(unsigned short id)
{
    cXMLElement* qosXml = NULL;
    if (par(PAR_QOSREQ).xmlValue() != NULL
            && par(PAR_QOSREQ).xmlValue()->hasChildren())
    {
        qosXml = par(PAR_QOSREQ).xmlValue();
    }
    else
    {
        return NULL;
    }

    cXMLElementList cubes = qosXml->getChildrenByTagName(ELEM_QOSREQ);
    for (cXMLElementList::iterator it = cubes.begin(); it != cubes.end(); ++it)
    {
        cXMLElement* m = *it;
        if (!m->getAttribute(ATTR_ID))
        {
            EV << "Error parsing QoSReq. Its ID is missing!" << endl;
            continue;
        }
        //Skipping QoSReqs with wrong ID
        else if ((unsigned short) atoi(m->getAttribute(ATTR_ID)) != id)
        {
            continue;
        }

        cXMLElementList attrs = m->getChildren();

        return new QoSReq(attrs);
    }
    return NULL;
}


/**
 * A convenience function for interconnecting two modules.
 * TODO: convert this into a global utility method so others can use it
 *
 * @param m1 first module
 * @param m2 second module
 * @param n1 first module gate name
 * @param n2 second module gate name
 */
void RA::interconnectModules(cModule* m1, cModule* m2, std::string n1, std::string n2)
{
    if (!m1->hasGate(n1.c_str()))
    {
        m1->addGate(n1.c_str(), cGate::INOUT, false);
    }
    cGate* m1In = m1->gateHalf(n1.c_str(), cGate::INPUT);
    cGate* m1Out = m1->gateHalf(n1.c_str(), cGate::OUTPUT);

    if (!m2->hasGate(n2.c_str()))
    {
        m2->addGate(n2.c_str(), cGate::INOUT, false);
    }
    cGate* m2In = m2->gateHalf(n2.c_str(), cGate::INPUT);
    cGate* m2Out = m2->gateHalf(n2.c_str(), cGate::OUTPUT);

    if (m2->getParentModule() == m1)
    {
        m1In->connectTo(m2In);
        m2Out->connectTo(m1Out);
    }
    else
    {
        m1Out->connectTo(m2In);
        m2Out->connectTo(m1In);
    }
}


/**
 * Connects the medium defined in NED to the RMT module.
 * Used only for bottom IPC processes in a computing systems.
 */
void RA::bindMediumToRMT()
{
    // retrieve the south gate
    cGate* thisIPCIn = thisIPC->gateHalf("southIo$i", cGate::INPUT, 0);
    cGate* thisIPCOut = thisIPC->gateHalf("southIo$o", cGate::OUTPUT, 0);

    //// connect bottom of this IPC to rmtModule
    // create an INOUT gate on the bottom of rmtModule
    std::ostringstream rmtGate;
    rmtGate << GATE_SOUTHIO_ << "PHY";
    rmtModule->addGate(rmtGate.str().c_str(), cGate::INOUT, false);
    cGate* rmtModuleIn = rmtModule->gateHalf(rmtGate.str().c_str(), cGate::INPUT);
    cGate* rmtModuleOut = rmtModule->gateHalf(rmtGate.str().c_str(), cGate::OUTPUT);

    rmtModuleOut->connectTo(thisIPCOut);
    thisIPCIn->connectTo(rmtModuleIn);

    // create a mock "(N-1)-port" for interface
    RMTPort* port = rmtAllocator->addPort(NULL);
    // connect the port to the bottom
    interconnectModules(rmtModule, port->getParentModule(), rmtGate.str(), std::string(GATE_SOUTHIO));
    // finalize initial port parameters
    port->postInitialize();
    port->setOutputReady();
    port->setInputReady();

    // apply queue allocation policy handler
    qAllocPolicy->onNM1PortInit(port);
}

/**
 * Connects the specified (N-1)-flow to the RMT.
 *
 * @param bottomIPC IPC process containing the (N-1)-flow
 * @param fab the (N-1)-FA
 * @param flow the (N-1)-flow
 * @return RMT port (handle) for the (N-1)-flow
 */
RMTPort* RA::bindNM1FlowToRMT(cModule* bottomIPC, FABase* fab, Flow* flow)
{
    // get (N-1)-port-id and expand it so it's unambiguous within this IPC
    int portID = flow->getSrcPortId();
    std::string combinedPortID = normalizePortID(bottomIPC->getFullName(), portID);

    // binding begins at the bottom and progresses upwards:
    // 1) interconnect bottom IPC <-> this IPC <-> compound RMT module
    std::ostringstream bottomIPCGate, thisIPCGate;
    bottomIPCGate << GATE_NORTHIO_ << portID;
    thisIPCGate << GATE_SOUTHIO_ << combinedPortID;
    interconnectModules(bottomIPC, thisIPC, bottomIPCGate.str(), thisIPCGate.str());
    interconnectModules(thisIPC, rmtModule, thisIPCGate.str(), thisIPCGate.str());

    // 2) attach a RMTPort instance (pretty much a representation of an (N-1)-port)
    RMTPort* port = rmtAllocator->addPort(flow);
    interconnectModules(rmtModule, port->getParentModule(), thisIPCGate.str(), std::string(GATE_SOUTHIO));
    // finalize initial port parameters
    port->postInitialize();

    // 3) allocate queues
    // apply queue allocation policy handler (if applicable)
    qAllocPolicy->onNM1PortInit(port);

    // 4) update the flow table
    flowTable->insert(flow, fab, port, thisIPCGate.str().c_str());

    return port;
}

/**
 * Prefixes given port-id (originally returned by an FAI) with IPC process's ID
 * to prevent name collisions in current IPC process.
 *
 * @param ipcName module identifier of an underlying IPC process
 * @param flowPortID original portId to be expanded
 * @return normalized port-id
 */
std::string RA::normalizePortID(std::string ipcName, int flowPortID)
{
    std::ostringstream newPortID;
    newPortID << ipcName << '_' << flowPortID;
    return newPortID.str();
}

/**
 * Invokes allocation of an (N)-flow (used for allocation of management flows).
 *
 * @param flow specified flow object
 */
void RA::createNFlow(Flow *flow)
{
    fa->receiveLocalMgmtAllocateRequest(flow);
}

/**
 * Invokes allocation of an (N-1)-flow (this is the mechanism behind Allocate() call).
 *
 * @param flow specified flow object
 */
void RA::createNM1Flow(Flow *flow)
{
    Enter_Method("createNM1Flow()");

    const APN& dstApn = flow->getDstApni().getApn();

    //
    // A flow already exists from this ipc to the destination one(passing through a neighbor)?
    //
    PDUFGNeighbor * e = fwdtg->getNextNeighbor(flow->getDstAddr(), flow->getConId().getQoSId());

    if(e)
    {
        NM1FlowTableItem * fi = flowTable->findFlowByDstAddr(
            e->getDestAddr().getApname().getName(),
            flow->getConId().getQoSId());

        if(fi)
        {
            return;
        }
    }
    //
    // End flow exists check.
    //

    //Ask DA which IPC to use to reach dst App
    const Address* ad = difAllocator->resolveApnToBestAddress(dstApn);
    if (ad == NULL)
    {
        EV << "DifAllocator returned NULL for resolving " << dstApn << endl;
        return;
    }
    Address addr = *ad;

    //TODO: Vesely - New IPC must be enrolled or DIF created
    if (!difAllocator->isDifLocal(addr.getDifName()))
    {
        EV << "Local CS does not have any IPC in DIF " << addr.getDifName() << endl;
        return;
    }

    // retrieve local IPC process enrolled in given DIF
    cModule* targetIPC = difAllocator->getDifMember(addr.getDifName());
    // retrieve the IPC process's Flow Allocator
    FABase* fab = difAllocator->findFaInsideIpc(targetIPC);
    // command the (N-1)-FA to allocate the flow
    bool status = fab->receiveAllocateRequest(flow);

    if (status)
    { // the Allocate procedure has successfully begun (and M_CREATE request has been sent)
        // bind the new (N-1)-flow to an RMT port
        RMTPort* port = bindNM1FlowToRMT(targetIPC, fab, flow);

        fab->invokeNewFlowRequestPolicy(flow);

        // notify the PDUFG of the new flow
        fwdtg->insertFlowInfo(
            Address(flow->getDstApni().getApn().getName()),
            flow->getQosCube(),
            port);
    }
    else
    { // Allocate procedure couldn't be invoked
       EV << "Flow not allocated!" << endl;
    }

    // flow creation will be finalized by postNM1FlowAllocation(flow)
    // (on arrival of M_CREATE_R)
}

/**
 * Handles receiver-side allocation of an (N-1)-flow requested by other IPC.
 * (this is the mechanism behind answering an M_CREATE request).
 *
 * @param flow specified flow object
 */
void RA::createNM1FlowWithoutAllocate(Flow* flow)
{
    Enter_Method("createNM1FlowWoAlloc()");

    const APN& dstAPN = flow->getDstApni().getApn();
    std::string qosID = flow->getConId().getQoSId();

    //
    // A flow already exists from this ipc to the destination one(passing through a neighbor)?
    //
    PDUFGNeighbor * e = fwdtg->getNextNeighbor(flow->getDstAddr(), flow->getConId().getQoSId());

    if(e)
    {
        NM1FlowTableItem * fi = flowTable->findFlowByDstAddr(
            e->getDestAddr().getApname().getName(),
            flow->getConId().getQoSId());

        if(fi)
        {
            return;
        }
    }
    //
    // End flow exists check.
    //


    // Ask DA which IPC to use to reach dst App
    const Address* ad = difAllocator->resolveApnToBestAddress(dstAPN);
    if (ad == NULL) {
        EV << "DifAllocator returned NULL for resolving " << dstAPN << endl;
        signalizeCreateFlowNegativeToRIBd(flow);
        return;
    }
    Address addr = *ad;

    //TODO: Vesely - New IPC must be enrolled or DIF created
    if (!difAllocator->isDifLocal(addr.getDifName()))
    {
        EV << "Local CS does not have any IPC in DIF " << addr.getDifName() << endl;
        signalizeCreateFlowNegativeToRIBd(flow);
        return;
    }

    // retrieve local IPC process enrolled in given DIF
    cModule* targetIpc = difAllocator->getDifMember(addr.getDifName());
    // retrieve the IPC process's Flow Allocator
    FABase* fab = difAllocator->findFaInsideIpc(targetIpc);
    // attach the new flow to RMT
    RMTPort* port = bindNM1FlowToRMT(targetIpc, fab, flow);
    // notify the PDUFG of the new flow
    fwdtg->insertFlowInfo(
        Address(flow->getDstApni().getApn().getName()),
        flow->getQosCube(),
        port);
    // answer the M_CREATE request
    signalizeCreateFlowPositiveToRIBd(flow);

    // mark this flow as connected
    flowTable->findFlowByDstApni(dstAPN.getName(), qosID)->
            setConnectionStatus(NM1FlowTableItem::CON_ESTABLISHED);
    port->setOutputReady();
    port->setInputReady();
}

/**
 * Event hook handler invoked after an (N)-flow is successfully established
 *
 * @param flow established (N)-flow
 */
void RA::postNFlowAllocation(Flow* flow)
{
    Enter_Method("postNFlowAllocation()");

    // invoke QueueAlloc policy on relevant (N-1)-ports
    if (rmt->isOnWire())
    {
        qAllocPolicy->onNFlowAlloc(rmtAllocator->getInterfacePort(), flow);
    }
    else
    {
        const std::string& neighApn = flow->getDstNeighbor().getApname().getName();
        std::string qosId = flow->getConId().getQoSId();

        NM1FlowTableItem* item = flowTable->findFlowByDstApni(neighApn, qosId);
        if (item != NULL)
        {
            qAllocPolicy->onNFlowAlloc(item->getRMTPort(), flow);
        }
    }
}

/**
 * Event hook handler invoked after an (N-1)-flow is successfully established
 *
 * @param flow established (N-1)-flow
 */
void RA::postNM1FlowAllocation(NM1FlowTableItem* ftItem)
{
    Enter_Method("postNM1FlowAllocation()");
    // mark this flow as connected and update info
    ftItem->setConnectionStatus(NM1FlowTableItem::CON_ESTABLISHED);
    RMTPort* port = ftItem->getRMTPort();
    port->setOutputReady();
    port->setInputReady();
    port->setFlow(ftItem->getFlow());

    // if this is a management flow, notify the Enrollment module
    if (ftItem->getFlow()->isManagementFlow())
    {
        signalizeMgmtAllocToEnrollment(ftItem->getFlow());
    }
}

/**
 * Removes specified (N-1)-flow and bindings (this is the mechanism behind Deallocate() call).
 *
 * @param flow specified flow object
 */
void RA::removeNM1Flow(Flow *flow)
{ // TODO: part of this should be split into something like postNM1FlowDeallocation

    NM1FlowTableItem* flowItem = flowTable->lookup(flow);
    ASSERT(flowItem != NULL);
    flowItem->setConnectionStatus(NM1FlowTableItem::CON_RELEASING);
    flowItem->getFABase()->receiveDeallocateRequest(flow);

    // disconnect and delete gates
    RMTPort* port = flowItem->getRMTPort();
    const char* gateName = flowItem->getGateName().c_str();
    cGate* thisIpcIn = thisIPC->gateHalf(gateName, cGate::INPUT);
    cGate* thisIpcOut = thisIPC->gateHalf(gateName, cGate::OUTPUT);
    cGate* rmtModuleIn = rmtModule->gateHalf(gateName, cGate::INPUT);
    cGate* rmtModuleOut = rmtModule->gateHalf(gateName, cGate::OUTPUT);
    cGate* portOut = port->getSouthOutputGate()->getNextGate();

    portOut->disconnect();
    rmtModuleOut->disconnect();
    thisIpcOut->disconnect();
    thisIpcIn->disconnect();
    rmtModuleIn->disconnect();

    rmtModule->deleteGate(gateName);
    thisIPC->deleteGate(gateName);

    // remove table entries
    fwdtg->removeFlowInfo(flowItem->getRMTPort());
    rmtAllocator->removePort(flowItem->getRMTPort());
    flowTable->remove(flow);
}

/**
 * Assigns a suitable (N-1)-flow to the (N)-flow (and creates one if there isn't any)
 *
 * @param flow specified flow object
 * @return true if an interface or an (N-1)-flow is ready to serve, false otherwise
 */
bool RA::bindNFlowToNM1Flow(Flow* flow)
{
    Enter_Method("bindNFlowToNM1Flow()");

    EV << "Received a request to bind an (N)-flow (dst "
       << flow->getDstApni().getApn().getName() << ", QoS-id "
       << flow->getConId().getQoSId() << ") to an (N-1)-flow." << endl;

    if (rmt->isOnWire())
    {
        EV << "This is a bottommost IPCP, the (N)-flow is bound to a medium." << endl;
        return true;
    }

    std::string dstAddr = flow->getDstAddr().getApname().getName();
    std::string neighAddr = flow->getDstNeighbor().getApname().getName();
    std::string qosID = flow->getConId().getQoSId();

    EV << "Binding to an (N-1)-flow leading to " << dstAddr;
    if (dstAddr != neighAddr)
    {
        EV << " through neighbor " << neighAddr;
    }
    EV << "." << endl;

    APNamingInfo srcAPN = APNamingInfo(APN(processName));
    APNamingInfo neighAPN = APNamingInfo(APN(neighAddr));
    APNamingInfo dstAPN = APNamingInfo(APN(dstAddr));




    PDUFGNeighbor * te =
        fwdtg->getNextNeighbor(flow->getDstAddr(), flow->getConId().getQoSId());

    if(te) {
        neighAddr = te->getDestAddr().getApname().getName();
    }

    NM1FlowTableItem* nm1FlowItem = flowTable->findFlowByDstApni(neighAddr, qosID);

    if (nm1FlowItem != NULL)
    { // a flow exists
        if (nm1FlowItem->getConnectionStatus() == NM1FlowTableItem::CON_ESTABLISHED)
        {
            EV << "Such (N-1)-flow is already present, using it." << endl;
            return true;
        }
        else if (nm1FlowItem->getConnectionStatus() == NM1FlowTableItem::CON_FLOWPENDING)
        {
            EV << "Such (N-1)-flow is already present but its allocation is not yet finished"
               << endl;
        }
    }
    else
    { // no suitable flow exists
        EV << "No such (N-1)-flow present, allocating a new one." << endl;
        Flow *nm1Flow = new Flow(srcAPN, neighAPN);
        nm1Flow->setQosRequirements(flow->getQosRequirements());
        createNM1Flow(nm1Flow);
    }

    return false;
}

void RA::blockNM1PortOutput(NM1FlowTableItem* ftItem)
{
    Enter_Method("blockNM1PortOutput()");
    ftItem->getRMTPort()->blockOutput();
}

void RA::unblockNM1PortOutput(NM1FlowTableItem* ftItem)
{
    Enter_Method("unblockNM1PortOutput()");
    ftItem->getRMTPort()->unblockOutput();
}

void RA::signalizeCreateFlowPositiveToRIBd(Flow* flow)
{
    emit(sigRACreFloPosi, flow);
}

void RA::signalizeCreateFlowNegativeToRIBd(Flow* flow)
{
    emit(sigRACreFloNega, flow);
}

void RA::signalizeSlowdownRequestToRIBd(cPacket* pdu)
{
    Enter_Method("signalizeSlowdownRequestToRIBd()");
    emit(sigRASDReqFromRMT, pdu);
}

void RA::signalizeSlowdownRequestToEFCP(cObject* obj)
{
    Enter_Method("signalizeSlowdownRequestToEFCP()");
    CongestionDescriptor* congInfo = check_and_cast<CongestionDescriptor*>(obj);
    emit(sigRASDReqFromRIB, congInfo);
}

void RA::signalizeMgmtAllocToEnrollment(Flow* flow)
{
    Enter_Method("signalizeMgmtAllocToEnrollment()");
    emit(sigRAMgmtAllocd, flow);
}

void RA::signalizeMgmtDeallocToEnrollment(Flow* flow)
{
    Enter_Method("signalizeMgmtDeallocToEnrollment()");
    emit(sigRAMgmtDeallocd, flow);
}

NM1FlowTable* RA::getFlowTable()
{
    return flowTable;
}
