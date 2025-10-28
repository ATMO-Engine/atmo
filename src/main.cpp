#include "atmo.hpp"
#include "core/components.hpp"
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

atmo::core::Engine engine;

int main(int argc, char **argv)
{
    std::signal(SIGINT, [](int signum) { engine.stop(); });
    std::signal(SIGTERM, [](int signum) { engine.stop(); });

    ECS_IMPORT(engine.get_ecs(), FlecsMeta);

    FileSystem::SetRootPath(get_executable_path());

    atmo::core::InputManager::AddEvent("#INTERNAL#ui_click", new atmo::core::InputManager::MouseButtonEvent(SDL_BUTTON_LEFT));

    atmo::core::InputManager::AddEvent("#INTERNAL#ui_scroll", new atmo::core::InputManager::MouseScrollEvent());


    auto window = engine.instantiate_prefab("window", "MainWindow");
    atmo::impl::WindowManager *wm = static_cast<atmo::impl::WindowManager *>(window.get_ref<atmo::core::ComponentManager::Managed>()->ptr);
    wm->rename("Atmo Engine");
    wm->make_main();

    while (engine.get_ecs().progress()) {
        atmo::core::InputManager::Tick();
    }

    return 0;
}
