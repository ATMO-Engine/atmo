#pragma once

#include <memory>

#include "core/ecs/entities/entity.hpp"
#include "editor/commands/commands.hpp"

#if !defined(ATMO_EXPORT)

namespace atmo::editor
{
    class IPlatformMenuBar
    {
    public:
        virtual ~IPlatformMenuBar() = default;

        virtual void build(core::ecs::entities::Entity window_entity, Commands &commands) = 0;
    };

    std::unique_ptr<IPlatformMenuBar> makePlatformMenuBar();
} // namespace atmo::editor

#endif
