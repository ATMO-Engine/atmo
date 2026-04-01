#include "engine.hpp"

#include <csignal>
#include <format>
#include <string>

#include "args/arg_manager.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/dynamic_2d/dynamic_2d.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/static_2d/static_2d.hpp"
#include "core/ecs/entities/2d/sprite_2d/sprite_2d.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/input/input_manager.hpp"
#include "core/resource/subresource_registry.hpp"
#include "core/resource/subresources/2d/shape/circle_shape2d.hpp"
#include "core/resource/subresources/2d/shape/rectangle_shape2d.hpp"
#include "core/types.hpp"
#include "glaze/json/write.hpp"
#include "project/file_system.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

#if !defined(ATMO_EXPORT)
#include "editor/editor.hpp"
#endif

static atmo::core::args::ArgManager::LaunchResult handleArgHelp(atmo::core::args::ArgManager &argManager)
{
#if !defined(ATMO_EXPORT)
    std::cout << ATMO_ASCII_ART << "\n";
#endif

    std::cout << argManager.Help().str() << "\n";

    return atmo::core::args::ArgManager::LaunchResult::ExitSuccess;
}

#if !defined(ATMO_EXPORT)
static atmo::core::args::ArgManager::LaunchResult handleArgPack(atmo::core::args::ArgManager &argManager)
{
    auto files = atmo::core::args::ArgManager::Get<std::vector<std::string>>("--pack");

    if (files.empty()) {
        spdlog::error("No files provided to pack.");
        return atmo::core::args::ArgManager::LaunchResult::ExitFailure;
    }

    atmo::project::ProjectManager::GeneratePackedFile("packed_output.pck", files);

    return atmo::core::args::ArgManager::LaunchResult::ExitSuccess;
}

static atmo::core::args::ArgManager::LaunchResult handleArgRead(atmo::core::args::ArgManager &argManager)
{
    auto args = atmo::core::args::ArgManager::Present<std::vector<std::string>>("--read");

    if (args->size() == 2)
        atmo::project::FileSystem::DisplayPackedFileContent(args->at(0), args->at(1));
    else if (args->size() == 1)
        atmo::project::FileSystem::DisplayPackedFileInfo(args->at(0));

    return atmo::core::args::ArgManager::LaunchResult::ExitSuccess;
}
#endif

namespace atmo::core
{
    int Engine::initLogger()
    {
#if defined(ATMO_DEBUG)
        spdlog::set_level(spdlog::level::debug);
#endif

        return 0;
    }

    int Engine::args(int argc, const char *const *argv)
    {
        using namespace atmo::core::args;

        ArgManager::AddArgument("--help", "-h").defaultValue(false).implicitValue(true).help("Show this help message.");
        ArgManager::AddLaunchHandler(10000, "--help", handleArgHelp);

#if !defined(ATMO_EXPORT)
        auto group = ArgManager::AddMutuallyExclusiveGroup();
        group.addArgument("--pack").nargs(ArgManager::NargsPattern::AtLeastOne).help("Pack one or more files into a .pck file.").metavar("files");
        ArgManager::AddLaunchHandler(9000, "--pack", handleArgPack);

        group.addArgument("--read")
            .nargs(1, 2)
            .help(
                "Read the contents of a .pck file. Provide a file path to display info, or provide a file path and an internal path to display the "
                "contents of a specific file.")
            .metavar("file [path]");
        ArgManager::AddLaunchHandler(9000, "--read", handleArgRead);
#endif

        try {
            ArgManager::Parse(argc, argv);
        } catch (const std::exception &e) {
            spdlog::error("Error parsing arguments: {}", e.what());
            return 1;
        }

        ArgManager::ExecuteLaunchHandlers();

        if (auto res = ArgManager::GetLaunchResult(); res != ArgManager::LaunchResult::Continue) {
            if (res == ArgManager::LaunchResult::ExitSuccess)
                return -1;
            else
                return 1;
        }

        return 0;
    }

    int Engine::initSDL()
    {
        if (SDL_Init(
                SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS | SDL_INIT_SENSOR |
                SDL_INIT_CAMERA) != true) {
            spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
            return 1;
        }

        std::atexit(SDL_Quit);

        return 0;
    }

    int Engine::initDefaultInputs()
    {
        using namespace atmo::core;

        InputManager::AddInput("ui_click", new InputManager::MouseButtonEvent(SDL_BUTTON_LEFT), true);
        InputManager::AddInput("ui_scroll", new InputManager::MouseScrollEvent(), true);
        InputManager::AddInput("ui_quit", new InputManager::KeyEvent(SDL_SCANCODE_ESCAPE, true), false);
        InputManager::AddInput("ui_confirm", new InputManager::KeyEvent(SDL_SCANCODE_RETURN, true), false);

        return 0;
    }

    int Engine::init()
    {
        if (int ret = initLogger(); ret != 0)
            return ret;

        if (int ret = initSDL(); ret != 0)
            return ret;

        if (int ret = initDefaultInputs(); ret != 0)
            return ret;

        static Engine &g_engine = *this;

        std::signal(SIGINT, [](int signum) { g_engine.stop(); });
        std::signal(SIGTERM, [](int signum) { g_engine.stop(); });

        m_ecs.reset();

        return 0;
    }

    static std::string get_window_name(float avgFps)
    {
        return std::format("{} - {:.0f} FPS", project::ProjectManager::GetSettings().app.project_name, avgFps);
    }

    void Engine::start()
    {
        m_running.store(true);

        auto window = ecs::EntityRegistry::Create<ecs::entities::Window>("Entity::Window");
        window->rename("_Root");
        window->setTitle(get_window_name(0.0f));
        window->onClose([&]() { stop(); });

        auto scene = ecs::EntityRegistry::Create<ecs::entities::Scene>("Entity::Scene");
        scene->setSingleton(false);
        m_ecs.changeScene(scene);

#if !defined(ATMO_EXPORT)
        editor::Editor editor(*this, "");
        editor.init();
#endif

        auto last_time = std::chrono::steady_clock::now();
        float deltaTime = 0.0f;
        float title_update_accumulator = 0.0f;
        int frame_count = 0;

        while (m_ecs.progress(deltaTime)) {
            InputManager::Tick();

            if (!m_running.load())
                m_ecs.stop();

            auto current_time = std::chrono::steady_clock::now();
            std::chrono::duration<float> dt = current_time - last_time;
            last_time = current_time;
            deltaTime = dt.count();
            title_update_accumulator += deltaTime;
            frame_count++;

            if (title_update_accumulator >= 0.5f) {
                window->setTitle(get_window_name(frame_count / title_update_accumulator));
                title_update_accumulator = 0.0f;
                frame_count = 0;
            }
        }
    }

    void Engine::stop()
    {
        m_running.store(false);
    }

    void Engine::reset()
    {
        m_ecs.reset();
    }

    ecs::ECS &Engine::getECS()
    {
        return m_ecs;
    }
} // namespace atmo::core
