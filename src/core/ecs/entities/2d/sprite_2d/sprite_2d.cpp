#include "sprite_2d.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void Sprite2d::init()
    {
        Entity2d::init();
        setComponent<components::Sprite2d>({});

        spdlog::info("Initialized Sprite2d entity with id {}", p_handle.id());
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Sprite2d);
