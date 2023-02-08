#ifndef SCIPCPP_SOLUTION_H
#define SCIPCPP_SOLUTION_H

#include <scipcpp/ForwardDeclarations.h>

class SCIPCPP::Solution
{
    friend class Solver;

    std::weak_ptr<SCIP*> m_scip;
    SCIP_SOL* m_solutionPointer;

    Solution(std::weak_ptr<SCIP*> instance, SCIP_SOL *solution);

public:

    Solution(Solution&& other);

    SCIP_RETCODE print(FILE* file = NULL, PrintZeros printZeros = PrintZeros::Yes);

};


#endif // SCIPCPP_SOLUTION_H
