#include "SceneHierarchy.hpp"
#include "spdlog/spdlog.h"

SceneHierarchy::SceneHierarchy(flecs::world &world)
    : world(world)
{
    spdlog::info("initialized: {}", world.to_json().c_str());
}

void SceneHierarchy::run()
{

}
