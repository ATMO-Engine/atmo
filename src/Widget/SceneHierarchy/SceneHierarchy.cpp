#include "SceneHierarchy.hpp"
#include "imgui.h"
#include "spdlog/spdlog.h"

SceneHierarchy::SceneHierarchy(const flecs::world &ecs) :
    ecs(ecs), query(ecs.query_builder<Engine>().term().first(flecs::ChildOf).oper(flecs::Not).cached().build())
{
}

static void log_entity(flecs::entity e, unsigned int depth)
{
    ImGui::Text("%s%s", std::string(depth * 2, ' ').c_str(), e.name().c_str());
    e.children([&](flecs::entity child) { log_entity(child, depth + 1); });
}

void SceneHierarchy::run()
{
    query.each([](flecs::entity e, Engine &engine) { log_entity(e, 0); });
}
