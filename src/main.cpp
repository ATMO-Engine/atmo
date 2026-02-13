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
    static atmo::core::Engine &g_engine = engine;

    std::signal(SIGINT, [](int signum) { g_engine.stop(); });
    std::signal(SIGTERM, [](int signum) { g_engine.stop(); });

    atmo::core::InputManager::AddInput("ui_click", new atmo::core::InputManager::MouseButtonEvent(SDL_BUTTON_LEFT), true);
    atmo::core::InputManager::AddInput("ui_scroll", new atmo::core::InputManager::MouseScrollEvent(), true);
    atmo::core::InputManager::AddInput("ui_quit", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_ESCAPE, true), false);
    atmo::core::InputManager::AddInput("ui_confirm", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_RETURN, true), false);

    atmo::core::InputManager::AddInput("rotate_left", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_Q, true), false);
    atmo::core::InputManager::AddInput("rotate_right", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_E, true), false);

    atmo::core::InputManager::AddInput("move_up", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_W, true), false);
    atmo::core::InputManager::AddInput("move_left", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_A, true), false);
    atmo::core::InputManager::AddInput("move_down", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_S, true), false);
    atmo::core::InputManager::AddInput("move_right", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_D, true), false);

    atmo::core::InputManager::AddInput("zoom_in", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_R, true), false);
    atmo::core::InputManager::AddInput("zoom_out", new atmo::core::InputManager::KeyEvent(SDL_SCANCODE_F, true), false);

    engine.start();

    return 0;
}
