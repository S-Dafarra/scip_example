#ifndef SCIPCPP_SOLVER_H
#define SCIPCPP_SOLVER_H

#include <scipcpp/ForwardDeclarations.h>
#include <scipcpp/Info.h>
#include <scipcpp/Solution.h>

class SCIPCPP::Solver
{
    std::shared_ptr<SCIP*> m_scipInternal{nullptr};

public:

    Solver();

    ~Solver();

    bool isValid() const;

    SCIP* get();

    Info info(FILE* file = NULL);

    SCIP_RETCODE printOrigProblem(FILE* file = NULL,
                                  const char* extension = NULL,
                                  SCIPCPP::UseGenericNames genericnames =  SCIPCPP::UseGenericNames::No);

    SCIP_RETCODE printTransProblem(FILE* file = NULL,
                                   const char* extension = NULL,
                                   SCIPCPP::UseGenericNames genericnames =  SCIPCPP::UseGenericNames::No);

    SCIP_RETCODE presolve();

    SCIP_RETCODE solve();

    int getNSols();

    Solution getBestSol();
};

#endif // SCIPCPP_SOLVER_H
