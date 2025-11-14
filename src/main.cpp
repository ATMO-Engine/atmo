#include <csignal>
#include <filesystem>
#include <iostream>
#include <string>
#include "atmo.hpp"
#include "core/ecs/components.hpp"
#include "core/event/Ievent.hpp"
#include "core/event/event_dispatcher.hpp"
#include "core/event/exemple_listener.hpp"
#include "editor/editor.hpp"
#include "editor/project_explorer.hpp"
#include "impl/window.hpp"

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

// Example event class test
class OtherEvent : public atmo::core::event::IEvent
{
public:
    OtherEvent(int value) : m_value(value) {}
    int getValue() const
    {
        return m_value;
    }

private:
    int m_value;
};


int main(int argc, char **argv)
{
    atmo::core::Engine engine;
    g_engine = &engine;

    std::signal(SIGINT, [](int signum) { g_engine->stop(); });
    std::signal(SIGTERM, [](int signum) { g_engine->stop(); });

    FileSystem::SetRootPath(get_executable_path());
    spdlog::info("Executable Path: {}", FileSystem::GetRootPath().string());

    atmo::core::InputManager::AddEvent("#INTERNAL#ui_click", new atmo::core::InputManager::MouseButtonEvent(SDL_BUTTON_LEFT));

    atmo::core::InputManager::AddEvent("#INTERNAL#ui_scroll", new atmo::core::InputManager::MouseScrollEvent());

    std::cout << "Dispatching EventExemple with value 42" << std::endl;

    atmo::core::event::EventExemple event(42);
    atmo::core::event::ExempleListener listener;

    std::cout << "Basic dispatch test:" << std::endl;
    atmo::core::event::EventDispatcher::getInstance().dispatch<atmo::core::event::EventExemple>(event);
    std::cout << "Listener called: " << (listener.called ? "true" : "false") << std::endl;

    listener.called = false;

    std::cout << "Dispatching OtherEvent test" << std::endl;
    atmo::core::event::EventDispatcher::getInstance().dispatch<OtherEvent>(event);
    std::cout << "Listener called: " << (listener.called ? "true" : "false") << std::endl;

    listener.called = false;

    std::cout << "Unsubscribing and dispatching EventExemple again" << std::endl;
    atmo::core::event::EventDispatcher::getInstance().unsubscribe<atmo::core::event::EventExemple>(listener);
    atmo::core::event::EventDispatcher::getInstance().dispatch<atmo::core::event::EventExemple>(event);
    std::cout << "Listener called: " << (listener.called ? "true" : "false") << std::endl;

    std::cout << "EventExemple ID  : " << atmo::core::event::event_id<atmo::core::event::EventExemple>() << "\n";
    std::cout << "OtherEvent ID    : " << atmo::core::event::event_id<OtherEvent>() << "\n";

    if (atmo::core::event::event_id<atmo::core::event::EventExemple>() == atmo::core::event::event_id<OtherEvent>()) {
        std::cout << "IDs identiques";
    } else {
        std::cout << "IDs différents.\n";
    }
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
