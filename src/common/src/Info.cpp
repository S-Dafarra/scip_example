#include <scipcpp/Info.h>


SCIPCPP::Info::Info(std::weak_ptr<SCIP *> instance, FILE *file)
    : m_scip(instance)
    , m_file(file)
{}

SCIPCPP::Info::Info(Info &&other)
    : std::stringstream(std::move(other))
    , m_scip(other.m_scip)
    , m_file(other.m_file)
{
    other.m_scip.reset();
}

SCIPCPP::Info::~Info()
{
    auto solverLocked = m_scip.lock();
    if (solverLocked)
        SCIPinfoMessage(*solverLocked.get(), m_file, "%s", this->str().c_str());

    m_scip.reset();
}
