#pragma once

#include <memory>
#include <string>

#include "core/ecs/entities/ui/ui.hpp"
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
        void makeProjectButton(core::ecs::entities::UI parent, std::shared_ptr<core::ecs::entities::UI> window_ui_container, std::string project_path);

    private:
        atmo::core::Engine &m_engine;
    };
} // namespace atmo::editor
#endif
