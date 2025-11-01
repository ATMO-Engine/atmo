#include "atmo.hpp"
#include "core/ecs/components.hpp"
#include "editor/editor.hpp"
#include "editor/project_explorer.hpp"
#include "impl/window.hpp"

#include <csignal>
#include <filesystem>
#include <string>

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
        atmo::core::InputManager::instance().tick();
    }
#endif
}

int main(int argc, char **argv)
{
    atmo::core::Engine engine;
    g_engine = &engine;

    std::signal(SIGINT, [](int signum) { g_engine->stop(); });
    std::signal(SIGTERM, [](int signum) { g_engine->stop(); });

    FileSystem::SetRootPath(get_executable_path());
    spdlog::info("Executable Path: {}", FileSystem::GetRootPath().string());

    atmo::core::InputManager::instance().addEvent("#INTERNAL#ui_click", new atmo::core::InputManager::MouseButtonEvent(SDL_BUTTON_LEFT));
    atmo::core::InputManager::instance().addEvent("#INTERNAL#ui_scroll", new atmo::core::InputManager::MouseScrollEvent());

    // auto window = engine.getECS().instantiatePrefab("window", "MainWindow");
    // atmo::impl::WindowManager *wm = static_cast<atmo::impl::WindowManager *>(window.get_ref<atmo::core::ComponentManager::Managed>()->ptr);
    // wm->rename("Atmo Engine");
    // wm->make_main();

    // while (g_engine->getECS().progress()) {
    //     atmo::core::InputManager::instance().tick();
    // }

    loop();

    return 0;
}
