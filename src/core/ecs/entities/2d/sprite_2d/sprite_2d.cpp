#include "sprite_2d.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    void Sprite2d::initComponents()
    {
        Entity2d::initComponents();
        setComponent<components::Sprite2d>({});
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Sprite2d);
