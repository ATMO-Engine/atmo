#include "entity.hpp"
#include <algorithm>
#include <iterator>
#include <memory>
#include "core/ecs/components.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entities/script.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/physics_progress_tick_event/physics_progress_tick_event.hpp"
#include "core/resource/resource.hpp"
#include "flecs/addons/cpp/c_types.hpp"
#include "flecs/addons/cpp/entity.hpp"
#include "glaze/glaze.hpp"
#include "luau/luau.hpp"
#include "meta/meta_registry.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    void Entity::RegisterSystems(flecs::world *world)
    {
        world->observer<components::EntityBase>("EntityBase_remove").event(flecs::OnRemove).each([](flecs::entity e, components::EntityBase &base) {
            for (auto &signal : base.signals) {
                signal.second->disconnectAll();
                delete signal.second;
                signal.second = nullptr;
            }
        });

        world->system<components::Script>().kind(flecs::OnUpdate).each([&](flecs::entity e, components::Script &script) {
            if (script.script_path.empty() || script.script_path == script.prev_script_path)
                return;

            script.prev_script_path = script.script_path;

            try {
                script.m_res = resource::ResourceManager::GetInstance().getResource<resource::Bytecode>(script.script_path);

                if (script.instance == nullptr)
                    script.instance = luau::Luau::Instance().generateInstance();

                spdlog::debug("Loaded script for entity {}: {}", e.name().c_str(), script.script_path);

                script.physics_event_id =
                    event::EventRegistry::SetCallBack<event::events::PhysicsProgressTickEvent>([e](event::events::PhysicsProgressTickEvent *evt) {
                        auto ent = Entity(e);
                        auto &script = ent.getComponentMutable<components::Script>();

                        if (script.instance)
                            script.instance->physicsUpdate(evt->delta_time);
                    });

                script.instance->load(script.script_path, script.m_res->get()->data, script.m_res->get()->size, e);
                script.instance->create();
            } catch (std::exception &e) {
                spdlog::error("Compilation error script not loaded: {}", e.what());
                return;
            }
        });

        world->system<components::Script>("Script_update").kind(flecs::OnValidate).each([](flecs::entity e, components::Script &script) {
            if (script.instance == nullptr || !script.m_res) {
                return;
            }
            float dt = e.world().delta_time();

            script.instance->update(dt);
        });

        world->observer<components::Script>("Script_remove").event(flecs::OnRemove).each([](flecs::entity e, components::Script &script) {
            if (script.script_path.empty())
                return;
            if (script.instance == nullptr || !script.m_res) {
                return;
            }

            event::EventRegistry::RemoveCallBack<event::events::PhysicsProgressTickEvent>(script.physics_event_id);

            script.instance->destroy();
            delete script.instance;
            script.m_res = nullptr;
        });
    }

    void Entity::Unregister(flecs::world *world) {}

    void Entity::setParent(Entity parent)
    {
        p_handle.child_of(parent.p_handle);
        parent.getSignal<Entity>("child_added").emit(*this);
    }

    void Entity::initialize()
    {
        createSignal<>("initialize");
        createSignal<Entity>("child_added");
        createSignal<Entity>("child_removed");
        createSignal<std::string, std::string>("renamed");
        p_handle.add(flecs::OrderedChildren);
    }

    EntityData Entity::serialize() const
    {
        EntityData output;
        const auto &comp = getComponent<components::EntityBase>();
        output.type = comp.type_name;
        output.name = p_handle.name();

        p_handle.each([&](flecs::id id) {
            if (id.is_pair())
                return;

            const meta::TypeInfo *ti = meta::MetaRegistry::Instance().findByFlecsId(id.raw_id());
            if (!ti || !ti->to_json)
                return;

            const void *comp = p_handle.try_get(id);
            if (!comp)
                return;

            output.components[ti->name] = ti->to_json(comp);
        });

        p_handle.children([&](flecs::entity child) {
            Entity wrapped{ child };
            output.children.emplace_back(wrapped.serialize());
        });

        return output;
    }

    void Entity::deserializeJson(std::string_view json)
    {
        EntityData data;
        if (glz::read_json(data, json))
            return;

        deserialize(data);
    }

    void Entity::deserialize(const EntityData &data)
    {
        rename(data.name);

        for (const auto &[comp_name, comp_json] : data.components) {
            const meta::TypeInfo *ti = meta::MetaRegistry::Instance().find(comp_name);
            if (!ti || !ti->from_json || ti->flecs_id == 0)
                continue;

            void *comp = p_handle.ensure(flecs::id(p_handle.world(), ti->flecs_id));
            if (!comp)
                continue;

            ti->from_json(comp, comp_json.dump().value());
        }

        for (const EntityData &child : data.children) {
            auto child_entity = ecs::EntityRegistry::Create(child.type);
            child_entity->deserialize(child);
            child_entity->setParent(*this);
        }
    }

    void Entity::deserializeInWorld(const EntityData &data, flecs::world *world)
    {
        rename(data.name);

        flecs::world entity_world = p_handle.world();

        for (const auto &[comp_name, comp_json] : data.components) {
            const meta::TypeInfo *ti = meta::MetaRegistry::Instance().find(comp_name);
            if (!ti || !ti->from_json || !ti->resolve_flecs_id)
                continue;

            const uint64_t local_id = ti->resolve_flecs_id(entity_world);
            if (local_id == 0)
                continue;

            void *comp = p_handle.ensure(flecs::id(entity_world, local_id));
            if (!comp)
                continue;

            ti->from_json(comp, comp_json.dump().value());
        }

        for (const EntityData &child : data.children) {
            auto child_entity = ecs::EntityRegistry::CreateIn(world, child.type);
            if (!child_entity)
                continue;
            child_entity->deserializeInWorld(child, world);
            child_entity->setParent(*this);
        }
    }

    flecs::entity Entity::getHandle() const
    {
        return p_handle;
    }

    std::vector<Entity> Entity::getChildren(bool recursive) const
    {
        std::vector<Entity> res;

        p_handle.children([&res, &recursive](flecs::entity child) {
            res.push_back(child);

            if (recursive) {
                Entity entity(child);

                std::vector<Entity> sub_res = entity.getChildren(recursive);
                res.reserve(res.size() + sub_res.size());
                res.insert(res.end(), sub_res.begin(), sub_res.end());
            }
        });

        return res;
    }

    Entity Entity::getChild(std::string_view name) const
    {
        return p_handle.lookup(name.data());
    }

    void Entity::destroy()
    {
        getParent().getSignal<Entity>("child_removed").emit(*this);
        p_handle.destruct();
    }

    bool Entity::isAlive() const
    {
        return p_handle.is_alive();
    }

    std::string_view Entity::name() const
    {
        return p_handle.name().c_str();
    }

    void Entity::rename(const std::string &new_name)
    {
        getSignal<std::string, std::string>("renamed").emit(p_handle.name().c_str(), new_name);
        p_handle.set_name(new_name.c_str());
    }

    bool Entity::isChildOf(Entity parent)
    {
        return p_handle.child_of(parent.p_handle);
    }

    std::shared_ptr<entities::Scene> Entity::getScene() const
    {
        flecs::entity current = p_handle;

        while (current.is_valid()) {
            if (current.has<components::Scene>()) {
                return std::make_shared<entities::Scene>(current);
            }
            current = current.parent();
        }

        return nullptr;
    }

    std::uint64_t Entity::getID() const
    {
        return p_handle.id();
    }

    void Entity::swap(const Entity &dest)
    {
        if (getParent() != dest.getParent())
            return;

        std::vector<flecs::entity_t> children;

        getParent().p_handle.children([&children](flecs::entity_t child) { children.push_back(child); });

        auto this_it = std::find(children.begin(), children.end(), p_handle);
        auto this_index = std::distance(children.begin(), this_it);
        auto dest_it = std::find(children.begin(), children.end(), dest.p_handle);
        auto dest_index = std::distance(children.begin(), dest_it);
        std::swap(children[this_index], children[dest_index]);

        getParent().p_handle.set_child_order(children.data(), static_cast<int32_t>(children.size()));
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Entity);
ATMO_REGISTER_COMPONENT(atmo::core::components::Script);
