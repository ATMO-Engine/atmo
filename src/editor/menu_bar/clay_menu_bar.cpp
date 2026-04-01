#include "clay_menu_bar.hpp"
#include "spdlog/spdlog.h"

#if !defined(ATMO_EXPORT)

#include "core/ecs/entities/entity.hpp"

namespace atmo::editor
{
    void ClayMenuBar::build(core::ecs::entities::Entity window_entity, Commands &commands)
    {
        spdlog::warn("The command menu bar for this platform has not been implemented yet.");
    }
} // namespace atmo::editor

#endif
