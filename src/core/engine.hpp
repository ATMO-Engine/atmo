#pragma once

#include <atomic>
#include <flecs.h>
#include <spdlog/spdlog.h>

#include "core/ecs/ecs.hpp"
#include "core/input/input_manager.hpp"
#include "project/project_manager.hpp"

namespace atmo
{
    namespace core
    {
        class Engine
        {
        private:
            ecs::ECS m_ecs;
            std::atomic<bool> m_running{ false };

        public:
            Engine() = default;
            ~Engine() = default;

            void start();
            void stop();
            void reset();
            ecs::ECS &getECS();
        };
    } // namespace core
} // namespace atmo
