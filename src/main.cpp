#include "atmo.hpp"

int main(int argc, char **argv)
{
    auto engine = atmo::core::Engine();

    spdlog::info("Engine initialized");
    auto window = engine.get_ecs().entity("Main Window").is_a(engine.get_prefabs().at("Window"));

    // while (engine.get_ecs().progress(0))
    //     continue;
    return 0;
}
