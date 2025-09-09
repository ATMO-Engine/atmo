#pragma once

#include <spdlog/spdlog.h>

#include "luau/luau.hpp"

namespace atmo
{
    namespace core
    {
        class Engine
        {
        protected:
            static inline Engine *singleton = nullptr;
            Engine() { singleton = this; }
            Engine(Engine &other) = delete;
            void operator=(const Engine &) = delete;

        public:
            static Engine *get_singleton() { return singleton; }
            void run() { spdlog::info("Engine is running!"); }
        };
    } // namespace core
} // namespace atmo
