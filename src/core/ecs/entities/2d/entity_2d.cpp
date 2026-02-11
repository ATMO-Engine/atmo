#include "entity_2d.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    void Entity2d::initComponents()
    {
        setComponent<components::Transform2d>({});
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Entity2d);
