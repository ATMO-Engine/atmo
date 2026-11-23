#pragma once

#include <memory>

#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "editor/commands/commands.hpp"

#if !defined(ATMO_EXPORT)

namespace atmo::editor
{
    class IPlatformMenuBar
    {
    public:
        virtual ~IPlatformMenuBar() = default;

        /**
         * @param window_entity Window the menu bar belongs to
         * @param commands Commands listed in the menus
         * @param container UI node the bar is attached to when the engine draws it (ignored by native menu bars)
         */
        virtual void build(core::ecs::entities::Window window_entity, Commands &commands, core::ecs::entities::UI container) = 0;
    };

    std::unique_ptr<IPlatformMenuBar> makePlatformMenuBar();
} // namespace atmo::editor

#endif
