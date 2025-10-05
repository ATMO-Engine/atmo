#include "atmo.hpp"
#include "impl/window.hpp"

#include <csignal>

atmo::core::Engine engine;

int main(int argc, char **argv)
{
    std::signal(SIGINT, [](int signum) { engine.stop(); });
    std::signal(SIGTERM, [](int signum) { engine.stop(); });

    auto window = engine.instantiate_prefab("window", "MainWindow");
    auto wm = static_cast<atmo::impl::WindowManager *>(engine.get_component_manager(window));

    while (engine.get_ecs().progress()) {
        continue;
    }

    return 0;
}
