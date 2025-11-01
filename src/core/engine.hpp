#pragma once

#include <flecs.h>
#include <spdlog/spdlog.h>

#include "core/ecs/ecs.hpp"
#include "core/input_manager.hpp"
#include "project/project_manager.hpp"

namespace atmo
{
    namespace core
    {
        class Engine
        {
        private:
            ecs::ECS m_ecs;

        public:
            Engine() = default;
            ~Engine() = default;

            void stop();
            void reset();
            ecs::ECS &getECS();
        };
    } // namespace core
} // namespace atmo
