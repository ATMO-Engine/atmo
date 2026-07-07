#pragma once

#include <atomic>
#include <flecs.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

#include "core/ecs/ecs.hpp"
#include "core/input/input_manager.hpp"
#include "project/project_manager.hpp"

namespace atmo::addon
{
    class Addon;
} // namespace atmo::addon

#if !defined(ATMO_EXPORT)
namespace atmo::editor
{
    class EditorManager;
} // namespace atmo::editor
#endif

namespace atmo::core
{
    class Engine
    {
    private:
        ecs::ECS m_ecs;
        std::atomic<bool> m_running{ false };
        bool m_headless = false;

        std::vector<std::unique_ptr<addon::Addon>> m_addons;

#if !defined(ATMO_EXPORT)
        std::unique_ptr<editor::EditorManager> m_editor;
#endif

        int initLogger();
        int initSDL();
        int initDefaultInputs();

        /**
         * @brief Loads every enabled addon declared in the currently opened project's settings.
         *
         * Must run after a project has been opened (ProjectManager::OpenProject), since addon
         * declarations live in that project's project_settings.json.
         */
        void loadAddons();

    public:
        Engine();
        ~Engine();

        int init();
        int args(int argc, const char *const *argv);

        void start();
        void stop();
        void reset();
        ecs::ECS &getECS();

#if !defined(ATMO_EXPORT)
        bool launchEditor(const std::string &project_path);

        static constexpr bool is_editor = true;
#else
        static constexpr bool is_editor = false;
#endif
    };
} // namespace atmo::core
