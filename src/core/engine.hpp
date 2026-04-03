#pragma once

#include <atomic>
#include <flecs.h>
#include <spdlog/spdlog.h>

#include "core/ecs/ecs.hpp"
#include "core/input/input_manager.hpp"
#include "project/project_manager.hpp"

namespace atmo::core
{
    class Engine
    {
    private:
        ecs::ECS m_ecs;
        std::atomic<bool> m_running{ false };
        bool m_headless = false;

        int initLogger();
        int initSDL();
        int initDefaultInputs();

    public:
        Engine() = default;
        ~Engine() = default;

        int init();
        int args(int argc, const char *const *argv);

        void start();
        void stop();
        void reset();
        ecs::ECS &getECS();

#if !defined(ATMO_EXPORT)
        static constexpr bool is_editor = true;
#else
        static constexpr bool is_editor = false;
#endif
    };
} // namespace atmo::core
