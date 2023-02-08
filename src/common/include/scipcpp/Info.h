#ifndef SCIPCPP_INFO_H
#define SCIPCPP_INFO_H

#include <scipcpp/ForwardDeclarations.h>

class SCIPCPP::Info : public std::stringstream
{
    std::weak_ptr<SCIP*> m_scip;
    FILE* m_file{nullptr};

    Info(std::weak_ptr<SCIP*> instance, FILE* file);

    friend class SCIPCPP::Solver;
public:

    Info(Info&& other);

    ~Info();
};

#endif // SCIPCPP_INFO_H
