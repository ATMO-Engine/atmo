#include "sprite_2d.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void Sprite2d::initialize()
    {
        Entity2d::initialize();
        setComponent<components::Sprite2d>({});
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Sprite2d);
