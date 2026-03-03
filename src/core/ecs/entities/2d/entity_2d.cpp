#include "entity_2d.hpp"
#include <memory>
#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    void Entity2d::RegisterComponents(flecs::world *world)
    {
        world->component<components::Transform2d>();
    }

    void Entity2d::RegisterSystems(flecs::world *world)
    {
        world->system<components::Transform2d, components::Transform2d>("Transform2d_GenerateGlobal")
            .kind(flecs::PreUpdate)
            .term_at(1)
            .up()
            .each([](flecs::entity e, components::Transform2d &t, const components::Transform2d &parent_t) {
                t.g_position = { parent_t.g_position.x + t.position.x, parent_t.g_position.y + t.position.y };
                t.g_rotation = parent_t.g_rotation + t.rotation;
                t.g_scale = { parent_t.g_scale.x * t.scale.x, parent_t.g_scale.y * t.scale.y };
            });
    }

    void Entity2d::initialize()
    {
        Entity::initialize();

        setComponent<components::Transform2d>({});
    }

    types::Vector2 Entity2d::getPosition() const
    {
        auto t = p_handle.get_ref<components::Transform2d>();
        return t->position;
    }

    void Entity2d::setPosition(const types::Vector2 &position)
    {
        auto t = p_handle.get_ref<components::Transform2d>();
        t->position = position;
    }

    types::Vector2 Entity2d::getGlobalPosition() const
    {
        auto t = p_handle.get_ref<components::Transform2d>();
        return t->g_position;
    }

    types::Vector2 Entity2d::getScale() const
    {
        auto t = p_handle.get_ref<components::Transform2d>();
        return t->scale;
    }

    void Entity2d::setScale(const types::Vector2 &scale)
    {
        auto t = p_handle.get_ref<components::Transform2d>();
        t->scale = scale;
    }

    types::Vector2 Entity2d::getGlobalScale() const
    {
        auto t = p_handle.get_ref<components::Transform2d>();
        return t->g_scale;
    }

    float Entity2d::getRotation() const
    {
        auto t = p_handle.get_ref<components::Transform2d>();
        return t->rotation;
    }

    void Entity2d::setRotation(float rotation)
    {
        auto t = p_handle.get_ref<components::Transform2d>();
        t->rotation = rotation;
    }

    float Entity2d::getGlobalRotation() const
    {
        auto t = p_handle.get_ref<components::Transform2d>();
        return t->g_rotation;
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Entity2d);
