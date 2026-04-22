#pragma once

#include "editor/menu_bar/i_platform_menu_bar.hpp"

#if !defined(ATMO_EXPORT)

namespace atmo::editor
{
    class ClayMenuBar : public IPlatformMenuBar
    {
    public:
        void build(core::ecs::entities::Entity window_entity, Commands &commands) override;
    };
} // namespace atmo::editor

#endif
