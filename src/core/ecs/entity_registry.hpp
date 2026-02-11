#pragma once

#include <string>
#include <type_traits>
#include <unordered_map>

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

            private:
                flecs::world *m_world{ nullptr };
            };
        } // namespace ecs
    } // namespace core
} // namespace atmo
