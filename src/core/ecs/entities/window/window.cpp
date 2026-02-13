#include "window.hpp"
#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    void Window::RegisterSystems(flecs::world *world) {}

    void Window::initialize()
    {
        Entity::initialize();
        setComponent<components::Transform2d>({});
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Window);
