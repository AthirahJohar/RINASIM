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

import rina.policies.DIF.RMT.Scheduler.IntRMTSchedulingPolicy;
import rina.policies.DIF.RMT.Monitor.IntRMTQMonitorPolicy;
import rina.policies.DIF.RMT.MaxQueue.IntRMTMaxQPolicy;
import rina.policies.DIF.RMT.PDUForwarding.IntPDUForwarding;

package rina.src.DIF.RMT;

module RelayAndMux
{
    parameters:
        @display("i=block/classifier;bgb=700,230");

        string schedPolicyName = default("LongestQFirst");
        string qMonitorPolicyName = default("DummyMonitor");
        string maxQPolicyName = default("TailDrop");
        string ForwardingPolicyName = default("SimpleTable");

        int defaultMaxQLength = default(20);
        int defaultThreshQLength = default(10);
        
        // PDU tracefile generation.
        // format: event time node ipcp pduType pduSize flags flow DIF srcAddr dstAddr seq id
        //
        // event: r (receive) / s (send) / + (enqueue) / - (dequeue) / d (drop)
        // time: event timestamp in seconds
        // node: node name
        // ipcp: IPC process name
        // pduType: PDU type
        // pduSize: PDU size in bits
        // flags: PDU flags
        // flow: flow-id (srcCEP + dstCEP + qosID)
        // DIF: DIF name
        // srcAddr: source address
        // dstAddr: destination address
        // seq: PDU sequence number
        // id: packet ID (unambiguous in scope of whole simulation)
        bool pduTracing = default(false);

    gates:
        inout ribdIo;

    submodules:
        rmt: RMT {
            @display("p=55,55");
        }
        schedulingPolicy: <schedPolicyName> like IntRMTSchedulingPolicy {
            @display("p=175,55;is=s");
        }
        queueMonitorPolicy: <qMonitorPolicyName> like IntRMTQMonitorPolicy {
            @display("p=295,55;is=s");
        }
        maxQueuePolicy: <maxQPolicyName> like IntRMTMaxQPolicy {
            @display("p=415,55;is=s");
        }
        pduForwardingPolicy: <ForwardingPolicyName> like IntPDUForwarding {
            @display("p=535,55;is=s");
        }
        allocator: RMTModuleAllocator {
            @display("p=655,55;i=block/cogwheel");
        }
    connections allowunconnected:
        ribdIo <--> rmt.ribdIo;
}
