#include <flecs.h>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include "Window/Window.hpp"

int main(int argc, char *argv[])
{
    flecs::world ecs;
    Window window;

    if (!window.init()) {
        spdlog::error("Failed to init window");
        return 1;
    }
    window.setupImGui();

    window.run();

    window.destroy();
    return 0;
}
