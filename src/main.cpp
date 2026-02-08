#include <atomic>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <variant>
#include "atmo.hpp"
#include "core/args/arg_manager.hpp"
#include "core/ecs/components.hpp"
#include "core/scene/scene_manager.hpp"
#include "editor/editor.hpp"
#include "editor/project_explorer.hpp"
#include "impl/window.hpp"
#include "locale/locale_manager.hpp"
#include "project/file_system.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

static std::atomic<bool> g_should_quit{ false };

static void loop(atmo::core::Engine &engine)
{
#if !defined(ATMO_EXPORT) // editor mode
    atmo::editor::ProjectExplorer project_explorer(&engine);
    project_explorer.loop();
    std::string selected_path = project_explorer.getSelectedPath();
    engine.reset();
    atmo::editor::Editor editor(&engine, selected_path);
    editor.init();
    editor.loop();
#else // export mode
    throw std::runtime_error("Not implemented yet.");
    while (engine.getECS().progress()) {
        ATMO_PROFILE_FRAME();
        atmo::core::InputManager::Tick();
    }
#endif
}

#if !defined(ATMO_EXPORT)
static int handleArgs()
{
    if (atmo::core::args::ArgManager::HasArg("help") || atmo::core::args::ArgManager::HasArg("h")) {
        std::cout << ATMO_ASCII_ART << std::endl;
        std::cout << "Atmo Engine Usage:" << std::endl;
        std::cout << "  --help, -h           Show this help message." << std::endl;
        std::cout << "  --pack files         Generate a .pck packed resource file from <files>." << std::endl;
        std::cout << "  --read path [file]   Read packed .pck resource file at <path> and output info." << std::endl;
        std::cout << "                         - if file is given, will output the contents of the file instead." << std::endl;
        return 1;
    }

    if (atmo::core::args::ArgManager::HasArg("pack")) {
        std::vector<std::string> files = atmo::core::args::ArgManager::GetNamedArgs("pack");

        files.emplace(files.begin(), std::get<std::string>(atmo::core::args::ArgManager::GetArgValue("pack")));

        if (files.empty()) {
            spdlog::error("No files provided to pack.");
            return -1;
        }

        atmo::project::ProjectManager::GeneratePackedFile("packed_output.pck", files);

        return 1;
    }

    if (atmo::core::args::ArgManager::HasArg("read")) {
        auto arg = atmo::core::args::ArgManager::GetArg("read");
        if (std::holds_alternative<bool>(arg.value)) {
            spdlog::error("No path provided to read.");
            return -1;
        }
        auto path = std::get<std::string>(arg.value);
        auto args = atmo::core::args::ArgManager::GetNamedArgs("read");
        if (args.size() == 1)
            atmo::project::FileSystem::DisplayPackedFileContent(path, args[0]);
        else
            atmo::project::FileSystem::DisplayPackedFileInfo(path);

        return 1;
    }

    return 0;
}
#endif

int main(int argc, char **argv)
{
#if defined(ATMO_DEBUG)
    spdlog::set_level(spdlog::level::debug);
#endif

    atmo::core::args::ArgManager::Parse(argc, argv);

#if !defined(ATMO_EXPORT)
    if (int res = handleArgs(); res != 0) {
        if (res > 0)
            res = 0;
        return res;
    }
#endif

    if (SDL_Init(
            SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS | SDL_INIT_SENSOR | SDL_INIT_CAMERA) !=
        true) {
        spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
        return 1;
    }

    std::atexit(SDL_Quit);

    atmo::core::Engine engine;

    std::signal(SIGINT, [](int signum) { g_should_quit.store(true); });
    std::signal(SIGTERM, [](int signum) { g_should_quit.store(true); });

    loop(engine);

    atmo::core::InputManager::AddInput("ui_click", new atmo::core::InputManager::MouseButtonEvent(SDL_BUTTON_LEFT), true);
    atmo::core::InputManager::AddInput("ui_scroll", new atmo::core::InputManager::MouseScrollEvent(), true);

    atmo::core::InputManager::AddInput("escape", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_ESCAPE, true), false);

    atmo::core::InputManager::AddInput("rotate_left", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_Q, true), false);
    atmo::core::InputManager::AddInput("rotate_right", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_E, true), false);

    atmo::core::InputManager::AddInput("move_up", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_W, true), false);
    atmo::core::InputManager::AddInput("move_left", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_A, true), false);
    atmo::core::InputManager::AddInput("move_down", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_S, true), false);
    atmo::core::InputManager::AddInput("move_right", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_D, true), false);

    atmo::core::InputManager::AddInput("zoom_in", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_R, true), false);
    atmo::core::InputManager::AddInput("zoom_out", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_F, true), false);

    auto window = engine.getECS().instantiatePrefab("window", "_Root");
    atmo::impl::WindowManager *wm = static_cast<atmo::impl::WindowManager *>(window.get<atmo::core::ComponentManager::Managed>().ptr);
    wm->rename(atmo::project::ProjectManager::GetSettings().app.project_name);

    auto scene = engine.getECS().instantiatePrefab("scene");
    auto sprite = engine.getECS().instantiatePrefab("sprite2d", "TestSprite");
    sprite.child_of(scene);
    sprite.set<atmo::core::components::Sprite2D>({ "project://assets/atmo.png" });
    auto sprite_transform = sprite.get_ref<atmo::core::components::Transform2D>();
    sprite_transform->position = { 100.0f, 100.0f };
    sprite_transform->scale = { 0.25f, 0.25f };

    engine.getECS().changeScene(scene);

    auto last_time = std::chrono::steady_clock::now();
    float deltaTime = 0.0f;

    while (engine.getECS().progress(deltaTime)) {
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<float> dt = current_time - last_time;
        last_time = current_time;
        deltaTime = dt.count();

        atmo::core::InputManager::Tick();

        if (atmo::core::InputManager::IsPressed("escape")) {
            spdlog::info("Escape pressed, quitting...");
            g_should_quit.store(true);
        }

        // if (atmo::core::InputManager::IsPressed("rotate_left"))
        //     sprite_transform->rotation -= 1.0f * deltaTime * 60.0f;

        // if (atmo::core::InputManager::IsPressed("rotate_right"))
        //     sprite_transform->rotation += 1.0f * deltaTime * 60.0f;

        // atmo::core::types::vector2 move_delta{ 0, 0 };
        // move_delta.y = (atmo::core::InputManager::IsPressed("move_up") ? -1 : 0) + (atmo::core::InputManager::IsPressed("move_down") ? 1 : 0);
        // move_delta.x = (atmo::core::InputManager::IsPressed("move_left") ? -1 : 0) + (atmo::core::InputManager::IsPressed("move_right") ? 1 : 0);
        // if (move_delta.x != 0 || move_delta.y != 0) {
        //     float length = std::sqrt(move_delta.x * move_delta.x + move_delta.y * move_delta.y);
        //     move_delta.x /= length;
        //     move_delta.y /= length;

        //     sprite_transform->position.x += move_delta.x * 500.0f * deltaTime;
        //     sprite_transform->position.y += move_delta.y * 500.0f * deltaTime;
        // }

        // float zoom_delta = (atmo::core::InputManager::IsPressed("zoom_in") ? 1 : 0) + (atmo::core::InputManager::IsPressed("zoom_out") ? -1 : 0);
        // if (zoom_delta != 0) {
        //     sprite_transform->scale.x += zoom_delta * 0.001f;
        //     sprite_transform->scale.y += zoom_delta * 0.001f;
        // }

        if (g_should_quit.load()) {
            engine.stop();
        }
    }

    return 0;
}
