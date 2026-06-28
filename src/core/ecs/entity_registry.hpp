#pragma once

#include <format>
#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/registry/hierarchic_registry.hpp"
#include "entities/entity.hpp"
#include "flecs.h"
#include "locale/locale_manager.hpp"
#include "spdlog/spdlog.h"

#define ATMO_REGISTER_ENTITY(entity)                \
    namespace                                       \
    {                                               \
        static int _ = [] {                         \
            using namespace atmo::core::ecs;        \
            EntityRegistry::RegisterType<entity>(); \
            return 0;                               \
        }();                                        \
    }

namespace atmo::core::ecs
{
    class EntityRegistry : public registry::HierarchicRegistry<EntityRegistry, entities::Entity>
    {
    public:
        static void SetWorld(flecs::world *world);

        template <typename Type> static void OnRegister()
        {
            Instance().m_registers.push_back({ .systems = &Type::RegisterSystems, .unregister = &Type::Unregister });

            if constexpr (!std::is_abstract_v<Type>) {
                Instance().m_wrap_factories[std::string(Type::FullName())] = [](flecs::entity h) -> entities::Entity * { return new Type(h); };
            }
        }

        template <typename Type> static entities::Entity *Factorize()
        {
            flecs::entity handle = Instance().m_world->entity();
            Type *entity = new Type(handle);

            entity->setComponent(components::EntityBase{ std::string(Type::FullName()) });
            entity->initialize();
            entity->rename(std::format("{}#{}", locale::LocaleManager::GetTranslation(std::format("atmo.entities.{}", Type::FullName())), handle.id()));

            return entity;
        }

        static std::unique_ptr<entities::Entity> Wrap(const entities::Entity &e);

        static void UnregisterAll(flecs::world *world);

        /**
         * @brief Create an entity of the given type inside a specific world, bypassing the global m_world pointer.
         *        Safe to call outside of world.progress() on the main thread only.
         *
         * @tparam T       Desired return type (must derive from Entity). Defaults to Entity.
         * @param world    Target world to create the entity in.
         * @param type_name Full type name (e.g. "Entity::Scene").
         * @return std::shared_ptr<T> The created entity cast to T, or nullptr on failure.
         */
        template <typename T = entities::Entity>
            requires std::derived_from<T, entities::Entity>
        static std::shared_ptr<T> CreateIn(flecs::world *world, std::string_view type_name)
        {
            auto &instance = Instance();
            flecs::world *saved = instance.m_world;
            instance.m_world = world;
            auto result = Create<T>(type_name);
            instance.m_world = saved;
            return result;
        }

        /**
         * @brief Register all entity systems into @p world without overwriting the main world pointer.
         *        Used by isolated worlds (e.g. per-SceneEditor ECS). Each entity type's RegisterSystems
         *        is responsible for checking WorldContext::is_editor_isolated to gate systems appropriately.
         *
         * @param world The isolated world to register systems into.
         */
        static void RegisterSystemsForWorld(flecs::world *world);

        /**
         * @brief Get the flecs entity from it's id
         *
         * @param id The id of the entity you want to retrieve
         * @return flecs::entity The flecs entity
         */
        static flecs::entity GetEntityFromId(uint64_t id);

    private:
        using WrapFactory = entities::Entity *(*)(flecs::entity);

        struct Register {
            std::function<void(flecs::world *)> systems;
            std::function<void(flecs::world *)> unregister;
        };

        flecs::world *m_world{ nullptr };
        std::vector<Register> m_registers;
        std::unordered_map<std::string, WrapFactory> m_wrap_factories;
    };
} // namespace atmo::core::ecs
