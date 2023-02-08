#include <scipcpp/Solver.h>

SCIPCPP::Solver::Solver()
{
    SCIP* newSolver;
    SCIP_CALL_ABORT(SCIPcreate(&newSolver));
    m_scipInternal = std::make_shared<SCIP*>(newSolver);
    SCIP_CALL_ABORT(SCIPincludeDefaultPlugins(*m_scipInternal));
}

SCIPCPP::Solver::~Solver()
{
    SCIP** scipPointer = m_scipInternal.get();
    SCIP_CALL_ABORT(SCIPfree(scipPointer));
    m_scipInternal = nullptr;
}

bool SCIPCPP::Solver::isValid() const
{
    return m_scipInternal.operator bool();
}

SCIP *SCIPCPP::Solver::get()
{
    return *m_scipInternal;
}

SCIPCPP::Info SCIPCPP::Solver::info(FILE *file)
{
    return Info(m_scipInternal, file);
}

SCIP_RETCODE SCIPCPP::Solver::printOrigProblem(FILE *file, const char *extension, UseGenericNames genericnames)
{
    return SCIPprintOrigProblem(*m_scipInternal, file, extension, static_cast<unsigned int>(genericnames));
}

SCIP_RETCODE SCIPCPP::Solver::printTransProblem(FILE *file, const char *extension, UseGenericNames genericnames)
{
    return SCIPprintTransProblem(*m_scipInternal, file, extension, static_cast<unsigned int>(genericnames));
}

SCIP_RETCODE SCIPCPP::Solver::presolve()
{
    return SCIPpresolve(*m_scipInternal);
}

SCIP_RETCODE SCIPCPP::Solver::solve()
{
    return SCIPsolve(*m_scipInternal);
}

int SCIPCPP::Solver::getNSols()
{
    return SCIPgetNSols(*m_scipInternal);
}

SCIPCPP::Solution SCIPCPP::Solver::getBestSol()
{
    return SCIPCPP::Solution(m_scipInternal, SCIPgetBestSol(*m_scipInternal));
}
