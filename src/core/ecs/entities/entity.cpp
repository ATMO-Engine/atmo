#include "entity.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    void Entity::init() {}

    std::vector<Entity> Entity::getChildren()
    {
        std::vector<Entity> res;

        p_handle.children([&res](flecs::entity child) { res.push_back(child); });

        return res;
    }

    Entity Entity::getChild(std::string_view name)
    {
        return p_handle.lookup(name.data());
    }

    Entity Entity::getParent()
    {
        return p_handle.parent();
    }

    void Entity::destroy()
    {
        p_handle.destruct();
    }

    bool Entity::isAlive()
    {
        return p_handle.is_valid();
    }

    // TODO: add way to change name of entity and create it with a name.
    std::string_view Entity::name() const
    {
        return p_handle.name().c_str();
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Entity);
