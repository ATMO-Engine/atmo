#include "project_explorer.hpp"

#if !defined(ATMO_EXPORT)
namespace atmo::editor
{
    ProjectExplorer::ProjectExplorer(atmo::core::Engine &engine) : m_engine(engine) {}

    void ProjectExplorer::init() {}
} // namespace atmo::editor
#endif
