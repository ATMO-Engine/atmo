#pragma once

#include "core/ecs/entities/window/window.hpp"
#include "editor/menu_bar/i_platform_menu_bar.hpp"

#if !defined(ATMO_EXPORT)

namespace atmo::editor
{
    class IntegratedMenuBar : public IPlatformMenuBar
    {
    public:
        void build(core::ecs::entities::Window window_entity, Commands &commands) override;
    };
} // namespace atmo::editor

#endif
