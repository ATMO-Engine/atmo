#pragma once

#include <flecs.h>
#include <spdlog/spdlog.h>

#include "core/entities.hpp"
#include "luau/luau.hpp"

#define ECS atmo::core::Engine::get_instance()->get_ecs()

namespace atmo
{
    namespace core
    {
        class Engine
        {
        protected:
            static inline Engine *singleton = nullptr;
            Engine()
            {
                singleton = this;
                load_ecs();
            }
            Engine(Engine &other) = delete;
            void operator=(const Engine &) = delete;

            void load_ecs();

            flecs::world ecs;
            static const std::map<std::string, flecs::entity> prefabs;

        public:
            static inline Engine *get_instance() { return singleton; }
            inline flecs::world &get_ecs() { return ecs; };
        };
    } // namespace core
} // namespace atmo
