#include <scipcpp/Solution.h>

SCIPCPP::Solution::Solution(std::weak_ptr<SCIP *> instance, SCIP_SOL* solution)
    : m_scip(instance)
    , m_solutionPointer(solution)
{
}

SCIPCPP::Solution::Solution(Solution &&other)
    : m_scip(other.m_scip)
    , m_solutionPointer(other.m_solutionPointer)
{
    other.m_scip.reset();
}

SCIP_RETCODE SCIPCPP::Solution::print(FILE *file, PrintZeros printZeros)
{
    auto instanceLocked = m_scip.lock();

    if (instanceLocked)
    {
        return SCIPprintSol(*instanceLocked, m_solutionPointer, file, static_cast<unsigned int>(printZeros));
    }

    return SCIP_ERROR;
}
