#include "entity_registry.hpp"

namespace atmo::core::ecs
{
    void EntityRegistry::SetWorld(flecs::world *world)
    {
        Instance().m_world = world;
    }
} // namespace atmo::core::ecs
