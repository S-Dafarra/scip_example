#ifndef SCIPCPP_FORWARDDECLARATIONS_H
#define SCIPCPP_FORWARDDECLARATIONS_H

#include <sstream>
#include <memory>

#include <scip/scip.h>
#include <scip/scipdefplugins.h>

namespace SCIPCPP {
    class Solver;
    class Solution;
    class Info;

    enum class YesNo : unsigned int{
        Yes = 1,
        No  = 0
    };

    using UseGenericNames = YesNo;
    using PrintZeros = YesNo;


}

#endif // SCIPCPP_FORWARDDECLARATIONS_H
