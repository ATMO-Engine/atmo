#include "atmo.hpp"

#include <csignal>

atmo::core::Engine engine;

int main(int argc, char **argv)
{
    std::signal(SIGINT, [](int signum) { engine.stop(); });
    std::signal(SIGTERM, [](int signum) { engine.stop(); });

    auto window = engine.instantiate_prefab("Window", "MainWindow");

    while (engine.get_ecs().progress()) {
        continue;
    }

    return 0;
}
