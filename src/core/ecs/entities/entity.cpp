#include "entity.hpp"
#include <memory>
#include "core/ecs/components.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    flecs::query<> Entity::m_SceneQuery;

    void Entity::RegisterComponents(flecs::world *world)
    {
        m_SceneQuery = world->query_builder("Entity_SceneLookupQuery").with<components::Scene>().up().build();
    }

    void Entity::RegisterSystems(flecs::world *world) {}

    void Entity::Unregister(flecs::world *world)
    {
        if (std::string_view(FullName()) == "Entity" && m_SceneQuery)
            m_SceneQuery.destruct();
    }

    void Entity::initialize() {}

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

    std::string_view Entity::name() const
    {
        return p_handle.name().c_str();
    }

    void Entity::rename(const std::string &new_name)
    {
        p_handle.set_name(new_name.c_str());
    }

    void Entity::loadFromJson(std::string_view json_data)
    {
        p_handle.from_json(json_data.data());
    }

    bool Entity::isChildOf(Entity parent)
    {
        return p_handle.child_of(parent.p_handle);
    }

    std::shared_ptr<entities::Scene> Entity::getScene() const
    {
        auto it = Entity::m_SceneQuery.iter(p_handle);

        if (it.is_true()) {
            entities::Scene *scene_entity = new entities::Scene{ it.first() };
            return std::shared_ptr<entities::Scene>(scene_entity);
        }

        return nullptr;
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Entity);
