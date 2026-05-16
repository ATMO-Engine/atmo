#pragma once

#include <string_view>

#include "core/engine.hpp"

#if !defined(ATMO_EXPORT)

namespace atmo::editor
{
    class ProjectExplorer
    {
    public:
        ProjectExplorer(atmo::core::Engine &engine);
        ~ProjectExplorer() = default;

        void init();

    private:
        atmo::core::Engine &m_engine;
    };
} // namespace atmo::editor
#endif
