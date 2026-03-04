#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "core/registry/hierarchic_registry.hpp"
#include "entities/entity.hpp"
#include "flecs.h"
#include "spdlog/spdlog.h"

#define REGISTER_ENTITY(entity)                     \
    namespace                                       \
    {                                               \
        static int _ = [] {                         \
            using namespace atmo::core::ecs;        \
            EntityRegistry::RegisterType<entity>(); \
            return 0;                               \
        }();                                        \
    }

namespace atmo
{
    namespace core
    {
        namespace ecs
        {
            class EntityRegistry : public registry::HierarchicRegistry<EntityRegistry, entities::Entity>
            {
            public:
                static void SetWorld(flecs::world *world);

                template <typename Type> static void OnRegister()
                {
                    Instance().m_registers.push_back(
                        { .components = &Type::RegisterComponents, .systems = &Type::RegisterSystems, .unregister = &Type::Unregister });
                }

                template <typename Type> static entities::Entity *Factorize()
                {
                    flecs::entity handle = Instance().m_world->entity();
                    Type *entity = new Type(handle);

                    entity->rename(std::format("{}#{}", Type::FullName(), handle.id()));
                    entity->initialize();

                    return entity;
                }

                static void UnregisterAll(flecs::world *world);

            private:
                struct Register {
                    std::function<void(flecs::world *)> components;
                    std::function<void(flecs::world *)> systems;
                    std::function<void(flecs::world *)> unregister;
                };

                flecs::world *m_world{ nullptr };
                std::vector<Register> m_registers;
            };
        } // namespace ecs
    } // namespace core
} // namespace atmo
