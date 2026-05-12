#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/registry/hierarchic_registry.hpp"
#include "entities/entity.hpp"
#include "flecs.h"
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

            entity->rename(std::format("{}#{}", Type::FullName(), handle.id()));
            entity->initialize();
            entity->setComponent(components::EntityType{ std::string(Type::FullName()) });

            return entity;
        }

        static std::unique_ptr<entities::Entity> Wrap(const entities::Entity &e);

        static void UnregisterAll(flecs::world *world);

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
