#include "atmo.hpp"
#include "impl/window.hpp"

#include <csignal>

atmo::core::Engine engine;

int main(int argc, char **argv)
{
    std::signal(SIGINT, [](int signum) { engine.stop(); });
    std::signal(SIGTERM, [](int signum) { engine.stop(); });

    atmo::core::InputManager::instance().addEvent(
        "#INTERNAL#ui_click", new atmo::core::InputManager::MouseButtonEvent(SDL_BUTTON_LEFT)
    );

    atmo::core::InputManager::instance().addEvent(
        "#INTERNAL#ui_scroll", new atmo::core::InputManager::MouseScrollEvent()
    );

    auto window = engine.instantiate_prefab("window", "MainWindow");
    auto wm = static_cast<atmo::impl::WindowManager *>(engine.get_component_manager(window));
    wm->rename("Atmo Engine");
    wm->make_main();

    while (engine.get_ecs().progress()) {
        atmo::core::InputManager::instance().tick();
    }

    return 0;
}
