#include "engine.hpp"

#include <csignal>
#include <format>
#include <string>

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_hints.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "args/arg_manager.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/dynamic_2d/dynamic_2d.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/static_2d/static_2d.hpp"
#include "core/ecs/entities/2d/sprite_2d/sprite_2d.hpp"
#include "core/ecs/entities/script.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/ecs/world_context.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/progress_tick_event/progress_tick_event.hpp"
#include "core/input/input_manager.hpp"
#include "core/resource/subresource_registry.hpp"
#include "core/resource/subresources/2d/shape/circle_shape2d.hpp"
#include "core/resource/subresources/2d/shape/rectangle_shape2d.hpp"
#include "core/types.hpp"
#include "impl/profiler.hpp"
#include "locale/locale_manager.hpp"
#include "luau/luau.hpp"
#include "luau/script_instance.hpp"
#include "project/file_system.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog-inl.h"
#include "spdlog/spdlog.h"

#if !defined(ATMO_EXPORT)
#include "addon/addon.hpp"
#include "editor/editor_manager.hpp"
#include "editor/project_explorer/project_explorer.hpp"
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

static atmo::core::args::ArgManager::LaunchResult handleArgExport(atmo::core::args::ArgManager &argManager)
{
    auto args = atmo::core::args::ArgManager::Get<std::vector<std::string>>("--export");
    auto project_path = atmo::core::args::ArgManager::Present<std::string>("--project");

    if (!project_path) {
        spdlog::error("--export requires --project <project_path>.");
        return atmo::core::args::ArgManager::LaunchResult::ExitFailure;
    }

    try {
        atmo::project::ProjectManager::OpenProject(*project_path);
    } catch (const std::exception &e) {
        spdlog::error("Failed to open project '{}': {}", *project_path, e.what());
        return atmo::core::args::ArgManager::LaunchResult::ExitFailure;
    }

    if (!atmo::project::ProjectManager::ExportProject(args.at(0), args.at(1)))
        return atmo::core::args::ArgManager::LaunchResult::ExitFailure;

    return atmo::core::args::ArgManager::LaunchResult::ExitSuccess;
}

static atmo::core::args::ArgManager::LaunchResult handleArgNewAddon(atmo::core::args::ArgManager &argManager)
{
    auto args = atmo::core::args::ArgManager::Get<std::vector<std::string>>("--new-addon");
    auto project_path = atmo::core::args::ArgManager::Present<std::string>("--project");

    if (!project_path) {
        spdlog::error("--new-addon requires --project <project_path>.");
        return atmo::core::args::ArgManager::LaunchResult::ExitFailure;
    }

    const std::string &name = args.at(0);
    const std::string &addon_template = args.at(1);

    if (addon_template != "assets" && addon_template != "library") {
        spdlog::error("--new-addon template must be 'assets' or 'library', got '{}'.", addon_template);
        return atmo::core::args::ArgManager::LaunchResult::ExitFailure;
    }

    try {
        atmo::project::ProjectManager::OpenProject(*project_path);
    } catch (const std::exception &e) {
        spdlog::error("Failed to open project '{}': {}", *project_path, e.what());
        return atmo::core::args::ArgManager::LaunchResult::ExitFailure;
    }

    try {
        std::filesystem::path addon_dir = atmo::addon::Addon::CreateTemplate(name, addon_template == "library");
        spdlog::info("Created addon at '{}'", addon_dir.string());
    } catch (const std::exception &e) {
        spdlog::error("Failed to create addon: {}", e.what());
        return atmo::core::args::ArgManager::LaunchResult::ExitFailure;
    }

    return atmo::core::args::ArgManager::LaunchResult::ExitSuccess;
}
#endif

static atmo::core::args::ArgManager::LaunchResult handleArgLang(atmo::core::args::ArgManager &argManager)
{
    atmo::locale::LocaleManager::SetUserLocale(atmo::core::args::ArgManager::Present<std::string>("--lang").value());
    return atmo::core::args::ArgManager::LaunchResult::Continue;
}

namespace atmo::core
{
    Engine::Engine() = default;
    Engine::~Engine() = default;

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

        ArgManager::AddArgument("--headless").defaultValue(false).implicitValue(true).help("Start in headless mode (no graphical interface)");

        ArgManager::AddArgument("--lang").help("Override the user's automatically detected preferred language.");
        ArgManager::AddLaunchHandler(0, "--lang", handleArgLang);

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

        group.addArgument("--project", "-p").nargs(1).help("Open editor for project at path").metavar("project_path");

        ArgManager::AddArgument("--run")
            .nargs(ArgManager::NargsPattern::Optional)
            .help(
                "Run the project as a standalone game instead of opening the editor. Loads the "
                "project's default scene unless an explicit scene path is given. Combine with --project "
                "to select which project to run.")
            .metavar("scene_path");

        ArgManager::AddArgument("--export")
            .nargs(2)
            .help(
                "Export the project given by --project into a standalone executable: copies "
                "<atmo_export_binary> to <output_path> and appends the project's packed asset data.")
            .metavar("atmo_export_binary output_path");
        ArgManager::AddLaunchHandler(9000, "--export", handleArgExport);

        ArgManager::AddArgument("--new-addon")
            .nargs(2)
            .help(
                "Create a new addon in the project given by --project, at addons/<name>: <name> <template>, "
                "where template is 'assets' for an asset-only addon with no shared library, or 'library' for "
                "an addon with a default xmake.lua that builds a shared library the engine can load.")
            .metavar("name assets|library");
        ArgManager::AddLaunchHandler(9000, "--new-addon", handleArgNewAddon);
#endif

        try {
            ArgManager::Parse(argc, argv);
        } catch (const std::exception &e) {
            spdlog::error("Error parsing arguments: {}", e.what());
            return 1;
        }

        return 0;
    }

    int Engine::initSDL()
    {
        static constexpr SDL_InitFlags flags =
            SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS | SDL_INIT_SENSOR | SDL_INIT_CAMERA;

        if (args::ArgManager::Get<bool>("--headless") == false && SDL_Init(flags)) {
            m_headless = false;
        } else {
            spdlog::info("booting in headless mode");
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

            if (!SDL_Init(flags)) {
                spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
                return 1;
            }

            m_headless = true;
        }

        if (!TTF_Init())
            spdlog::error("Failed to initialize SDL_TTF: {}", SDL_GetError());

        std::atexit(SDL_Quit);

        return 0;
    }

    int Engine::initDefaultInputs()
    {
        using namespace atmo::core;

        InputManager::AddInput("ui_click", new InputManager::MouseButtonEvent(SDL_BUTTON_LEFT), true);
        InputManager::AddInput("ui_rightClick", new InputManager::MouseButtonEvent(SDL_BUTTON_RIGHT), true);
        InputManager::AddInput("ui_scroll", new InputManager::MouseScrollEvent(), true);
        InputManager::AddInput("ui_pinch", new InputManager::PinchEvent(), true);
        InputManager::AddInput("ui_quit", new InputManager::KeyEvent(SDL_SCANCODE_ESCAPE, true), true);
        InputManager::AddInput("ui_confirm", new InputManager::KeyEvent(SDL_SCANCODE_RETURN, true), true);
        InputManager::AddInput("ui_delete", new InputManager::KeyEvent(SDL_SCANCODE_BACKSPACE, true), true);

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

        args::ArgManager::ExecuteLaunchHandlers();

        if (auto res = args::ArgManager::GetLaunchResult(); res != args::ArgManager::LaunchResult::Continue) {
            if (res == args::ArgManager::LaunchResult::ExitSuccess)
                return -1;
            else
                return 1;
        }

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

#if !defined(ATMO_EXPORT)
    bool Engine::launchEditor(const std::string &project_path)
    {
        try {
            project::ProjectManager::OpenProject(project_path);
        } catch (const std::exception &e) {
            spdlog::error("Failed to open project '{}': {}", project_path, e.what());
            return false;
        }

        m_editor = std::make_unique<editor::EditorManager>(*this, project::ProjectManager::GetCurrentProjectPath().string());
        m_editor->init();
        return true;
    }
#endif

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

        InputManager::AddInput("D", new InputManager::KeyEvent(SDLK_D, false));
        InputManager::AddInput("Q", new InputManager::KeyEvent(SDLK_Q, false));
        InputManager::AddInput("Space", new InputManager::KeyEvent(SDLK_SPACE, false));

#if !defined(ATMO_EXPORT)
        if (args::ArgManager::IsUsed("--run")) {
            if (std::optional<std::string> project_path = args::ArgManager::Present<std::string>("--project")) {
                try {
                    project::ProjectManager::OpenProject(*project_path);
                } catch (const std::exception &e) {
                    spdlog::error("Failed to open project '{}': {}", *project_path, e.what());
                    return;
                }
                project::FileSystem::SetProjectRootOverride(std::filesystem::absolute(project::ProjectManager::GetCurrentProjectPath()));
            }

            std::string scene_path = args::ArgManager::Present<std::string>("--run").value_or(project::ProjectManager::GetSettings().app.default_scene);

            if (scene_path.empty())
                spdlog::warn("Run mode: no scene to load (no --run argument and no app.default_scene configured).");
            else
                m_ecs.changeSceneToFile(scene_path);
        } else if (std::optional<std::string> project_path = args::ArgManager::Present<std::string>("--project")) {
            if (!launchEditor(*project_path))
                return;
        } else {
            editor::ProjectExplorer explorer(*this);
            explorer.init();
        }
#else
        {
            const std::string &scene_path = project::ProjectManager::GetSettings().app.default_scene;
            if (!scene_path.empty())
                m_ecs.changeSceneToFile(scene_path);
        }
#endif

        auto last_time = std::chrono::steady_clock::now();
        float deltaTime = 0.0f;
        float title_update_accumulator = 0.0f;
        int frame_count = 0;

        auto progress_tick = event::EventRegistry::Create<event::events::ProgressTickEvent>("Event::ProgressTickEvent");

        while (m_ecs.progress(deltaTime)) {
            ATMO_PROFILE_FRAME();

            if (!m_running.load()) {
                m_ecs.stop();
                continue;
            }

            SignalQueue::Flush();

            progress_tick->delta_time = deltaTime;
            event::EventRegistry::Dispatch(progress_tick);

            InputManager::Tick();

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
