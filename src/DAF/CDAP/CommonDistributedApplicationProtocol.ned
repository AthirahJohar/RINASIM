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

package rina.src.DAF.CDAP;

module CommonDistributedApplicationProtocol
{
    parameters:
        @display("bgb=331,242;i=block/circle");
        int currentInvokeId = (intuniform(0,65535)*1000);
    gates:
        inout southIo;

    submodules:
        cace: CACE {
            @display("p=63,59");
        }
        auth: Auth {
            @display("p=163,60");
        }
        cdap: CDAP {
            @display("p=260,60");
        }
        cdapSplitter: CDAPSplitter {
            @display("p=163,156");
        }

        cdapMsgLog: CDAPMsgLog {
            @display("p=260,156");
        }

    connections allowunconnected:
        cace.splitterIo <--> cdapSplitter.caceIo;
        auth.splitterIo <--> cdapSplitter.authIo;
        cdap.splitterIo <--> cdapSplitter.cdapIo;
        cdapSplitter.southIo <--> southIo;
}
