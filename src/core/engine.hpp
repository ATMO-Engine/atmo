#pragma once

#include <atomic>
#include <cstdint>
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

namespace atmo::mcp
{
    class McpServer;
} // namespace atmo::mcp
#endif

namespace atmo::core
{
    class Engine
    {
    private:
        struct RenderingServerShutdown {
            RenderingServerShutdown() = default;
            ~RenderingServerShutdown();

            RenderingServerShutdown(const RenderingServerShutdown &) = delete;
            RenderingServerShutdown &operator=(const RenderingServerShutdown &) = delete;
        };

        struct AudioServerShutdown {
            AudioServerShutdown() = default;
            ~AudioServerShutdown();

            AudioServerShutdown(const AudioServerShutdown &) = delete;
            AudioServerShutdown &operator=(const AudioServerShutdown &) = delete;
        };

        RenderingServerShutdown m_rendering_server_shutdown;
        AudioServerShutdown m_audio_server_shutdown;
        ecs::ECS m_ecs;
        std::atomic<bool> m_running{ false };
        bool m_headless = false;
        std::uint64_t m_frame = 0;
        bool m_paused = false;
        int m_step_frames = 0;
        float m_step_delta = 1.0f / 60.0f;

        std::vector<std::unique_ptr<addon::Addon>> m_addons;

#if !defined(ATMO_EXPORT)
        std::unique_ptr<editor::EditorManager> m_editor;
        std::unique_ptr<mcp::McpServer> m_mcp;

        void startMcp(bool game_mode);
#endif

        int initLogger();
        int initSDL();
        int initDefaultInputs();
        float applyTimeControl(float delta_time);

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

        bool isHeadless() const
        {
            return m_headless;
        }

        std::uint64_t frameIndex() const
        {
            return m_frame;
        }

        bool isPaused() const
        {
            return m_paused;
        }

        int pendingSteps() const
        {
            return m_step_frames;
        }

        void setPaused(bool paused);
        void step(int frames, float delta_time);

#if !defined(ATMO_EXPORT)
        bool launchEditor(const std::string &project_path);

        editor::EditorManager *editor() const
        {
            return m_editor.get();
        }

        static constexpr bool is_editor = true;
#else
        static constexpr bool is_editor = false;
#endif
    };
} // namespace atmo::core
