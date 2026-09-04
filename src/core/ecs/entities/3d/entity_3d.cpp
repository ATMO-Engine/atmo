#include "entity_3d.hpp"
#include <memory>
#include "core/ecs/components.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void Entity3d::RegisterSystems(flecs::world *world)
    {
        world->system<components::Transform3d>("Transform3d_IdentityGlobal").kind(flecs::OnValidate).each([](flecs::entity e, components::Transform3d &t) {
            flecs::entity parent = e.parent();
            if (parent.is_valid() && parent.has<components::Transform3d>())
                return;

            t.g_position = t.position;
            t.g_rotation = t.rotation;
            t.g_scale = t.scale;
        });

        world->system<components::Transform3d, components::Transform3d>("Transform3d_GenerateGlobal")
            .kind(flecs::OnValidate)
            .term_at(1)
            .up()
            .cascade(flecs::ChildOf)
            .each([](flecs::entity e, components::Transform3d &t, const components::Transform3d &parent_t) {
                t.g_position = { parent_t.g_position.x + t.position.x, parent_t.g_position.y + t.position.y };
                t.g_rotation = parent_t.g_rotation + t.rotation;
                t.g_scale = { parent_t.g_scale.x * t.scale.x, parent_t.g_scale.y * t.scale.y };
            });
    }

    void Entity3d::initialize()
    {
        Entity::initialize();

        setComponent<components::Transform3d>({});
    }

    types::Vector3 Entity3d::getPosition() const
    {
        auto t = p_handle.get_ref<components::Transform3d>();
        return t->position;
    }

    void Entity3d::setPosition(const types::Vector3 &position)
    {
        auto t = p_handle.get_ref<components::Transform3d>();
        t->position = position;
    }

    types::Vector3 Entity3d::getGlobalPosition() const
    {
        auto t = p_handle.get_ref<components::Transform3d>();
        return t->g_position;
    }

    types::Vector3 Entity3d::getScale() const
    {
        auto t = p_handle.get_ref<components::Transform3d>();
        return t->scale;
    }

    void Entity3d::setScale(const types::Vector3 &scale)
    {
        auto t = p_handle.get_ref<components::Transform3d>();
        t->scale = scale;
    }

    types::Vector3 Entity3d::getGlobalScale() const
    {
        auto t = p_handle.get_ref<components::Transform3d>();
        return t->g_scale;
    }

    float Entity3d::getRotation() const
    {
        auto t = p_handle.get_ref<components::Transform3d>();
        return t->rotation;
    }

    void Entity3d::setRotation(float rotation)
    {
        auto t = p_handle.get_ref<components::Transform3d>();
        t->rotation = rotation;
    }

    float Entity3d::getGlobalRotation() const
    {
        auto t = p_handle.get_ref<components::Transform3d>();
        return t->g_rotation;
    }

    SDL_FRect Entity3d::computeAABB() const
    {
        // auto t = p_handle.get_ref<components::Transform3d>();
        // constexpr float k_default_half = 10.f;
        // return { t->g_position.x - k_default_half, t->g_position.y - k_default_half, k_default_half * 2.f, k_default_half * 2.f };
        return {};
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY_COLOR(entities::Entity3d, atmo::core::types::Color("#F35151"));
ATMO_REGISTER_COMPONENT(atmo::core::components::Transform3d);
