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

#ifndef LatGenerator_H_
#define LatGenerator_H_

#include <IntPDUFG.h>
#include <MiniTable/MiniTable.h>
#include <SimpleRouting/IntSimpleRouting.h>

#include <map>
#include <set>

namespace LatGenerator {

struct portMetric {
    RMTPort* port;
    unsigned short metric;

    portMetric(RMTPort* p, unsigned short m);

    bool operator < (const portMetric &other) const;
};


typedef std::set<portMetric> PortsSet;
typedef std::map<std::string, PortsSet> NTable;

typedef PortsSet::iterator PortsSetIt;
typedef NTable::iterator NTableIt;


class LatGenerator: public IntPDUFG {
public:
    // A new flow has been inserted/or removed
    virtual void insertedFlow(const Address &addr, const QoSCube& qos, RMTPort * port);
    virtual void removedFlow(const Address &addr, RMTPort * port);

    //Routing has processes a routing update
    virtual void routingUpdated();

protected:
    // Called after initialize
    virtual void onPolicyInit();

private:
    DA * difA;
    MiniTable::MiniTable * fwd;
    IntSimpleRouting * rt;

    NTable neighbours;
};

}

#endif /* STATICGENERATOR_H_ */
