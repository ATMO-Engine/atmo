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
                    Instance().m_registers.push_back({ .components = &Type::RegisterComponents, .systems = &Type::RegisterSystems });
                }

                template <typename Type> static entities::Entity *Factorize()
                {
                    Type *entity = new Type(Instance().m_world->entity());

                    entity->initialize();

                    return entity;
                }

            private:
                struct Register {
                    std::function<void(flecs::world *)> components;
                    std::function<void(flecs::world *)> systems;
                };

                flecs::world *m_world{ nullptr };
                std::vector<Register> m_registers;
            };
        } // namespace ecs
    } // namespace core
} // namespace atmo
