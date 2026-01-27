#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include "atmo.hpp"
#include "core/args/arg_manager.hpp"
#include "core/ecs/components.hpp"
#include "core/scene/scene_manager.hpp"
#include "editor/editor.hpp"
#include "editor/project_explorer.hpp"
#include "impl/window.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

static std::filesystem::path get_executable_path()
{
#if defined(_WIN32)
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer);
#elif defined(__APPLE__)
    char buffer[4096];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0)
        return std::filesystem::canonical(buffer);
    std::string dyn(size, '\0');
    _NSGetExecutablePath(dyn.data(), &size);
    return std::filesystem::canonical(dyn.c_str());
#else
    char buffer[4096];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::filesystem::path(buffer);
    }
    return std::filesystem::current_path();
#endif
}

static atmo::core::Engine *g_engine;

static void loop()
{
#if !defined(ATMO_EXPORT) // editor mode
    atmo::editor::ProjectExplorer project_explorer(g_engine);
    project_explorer.loop();
    std::string selected_path = project_explorer.getSelectedPath();
    g_engine->reset();
    atmo::editor::Editor editor(g_engine, selected_path);
    editor.loop();
#else // export mode
    throw std::runtime_error("Not implemented yet.");
    while (g_engine->getECS().progress()) {
        atmo::core::InputManager::Tick();
    }
#endif
}

static int handleArgs()
{
    if (atmo::core::args::ArgManager::HasArg("help") || atmo::core::args::ArgManager::HasArg("h")) {
#if !defined(ATMO_EXPORT)
        std::cout << ATMO_ASCII_ART << std::endl;
        std::cout << "Atmo Engine Usage:" << std::endl;
        std::cout << "  --help, -h           Show this help message" << std::endl;
        std::cout << "  --pack <files>       Generate a .pck packed resource file from <files>" << std::endl;
        std::cout << "  --read <path>        Read packed .pck resource file at <path> and output info" << std::endl;
#endif
        return 1;
    }

    if (atmo::core::args::ArgManager::HasArg("pack")) {
        std::vector<std::string> files = atmo::core::args::ArgManager::GetNamedArgs("pack");

        files.emplace(files.begin(), std::get<std::string>(atmo::core::args::ArgManager::GetArgValue("pack")));

        if (files.empty()) {
            spdlog::error("No files provided to pack.");
            return 1;
        }

        atmo::project::ProjectManager::GeneratePackedFile("packed_output.pck", files);

        return 1;
    }

    if (atmo::core::args::ArgManager::HasArg("read")) {
        auto path = std::get<std::string>(atmo::core::args::ArgManager::GetArgValue("read"));

        atmo::project::ProjectManager::DisplayPackedFileInfo(path);

        return 1;
    }

    loop();

    return 0;
}

int main(int argc, char **argv)
{
#if defined(ATMO_DEBUG)
    spdlog::set_level(spdlog::level::debug);
#endif

    atmo::core::args::ArgManager::Parse(argc, argv);

    atmo::project::FileSystem::SetRootPath(get_executable_path());
    spdlog::debug("Executable Path: {}", atmo::project::FileSystem::GetRootPath().string());

    if (int res = handleArgs(); res != 0) {
        return res;
    }

    if (SDL_Init(
            SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS | SDL_INIT_SENSOR | SDL_INIT_CAMERA) !=
        true) {
        spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
        return 1;
    }

    std::atexit(SDL_Quit);

    atmo::core::Engine engine;
    g_engine = &engine;

    std::signal(SIGINT, [](int signum) { g_engine->stop(); });
    std::signal(SIGTERM, [](int signum) { g_engine->stop(); });

    atmo::core::InputManager::AddInput("ui_click", new atmo::core::InputManager::MouseButtonEvent(SDL_BUTTON_LEFT), true);
    atmo::core::InputManager::AddInput("ui_scroll", new atmo::core::InputManager::MouseScrollEvent(), true);

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
    sprite.set<atmo::core::components::Sprite2D>({ "assets/atmo.png" });
    auto sprite_transform = sprite.get_ref<atmo::core::components::Transform2D>();
    sprite_transform->position = { 100.0f, 100.0f };
    sprite_transform->scale = { 0.25f, 0.25f };

    engine.getECS().changeScene(scene);

    auto last_time = std::chrono::steady_clock::now();
    float deltaTime = 0.0f;
    while (g_engine->getECS().progress(deltaTime)) {
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<float> dt = current_time - last_time;
        last_time = current_time;
        deltaTime = dt.count();

        atmo::core::InputManager::Tick();

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
    }

    return 0;
}
