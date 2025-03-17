#include "SceneHierarchy.hpp"
#include <cfloat>
#include "imgui.h"
#include "spdlog/spdlog.h"

SceneHierarchy::SceneHierarchy(flecs::world &ecs, flecs::entity_t &selectedEntity) :
    ecs(ecs), query(ecs.query_builder<Node>().cached().build()), selectedEntity(selectedEntity)
{
}

void SceneHierarchy::logEntity(flecs::entity e, unsigned int depth)
{
    const std::string deleteBtn = std::format("x##{}", e.id());
    const std::string name = std::format("{}[{}] {}", std::string(2 * depth, ' '), e.id(), e.name().c_str());
    if (ImGui::Selectable(name.c_str(), selectedEntity == e.id()))
        selectedEntity = e.id();
    e.children([&](flecs::entity child) { logEntity(child, depth + 1); });
}

void SceneHierarchy::run()
{
    ImGui::Text("Scene Hierarchy");
    ImGui::SameLine();
    if (ImGui::Button("+"))
        EntityCreator::createEntity(ecs, "");

    ImGui::BeginChild("Entities", ImVec2(0, 0), false);
    ImGui::BeginListBox("##empty", ImVec2(-FLT_MIN, -FLT_MIN));
    query.each(
        [this](flecs::entity e, Node &node)
        {
            if (e.is_valid() && !e.parent() && e.is_alive())
                logEntity(e, 0);
        });
    ImGui::EndListBox();
    ImGui::EndChild();
    while (!deleteQueue.empty()) {
        flecs::entity_t id = deleteQueue.dequeue();
        ecs.entity(id).destruct();
    }
}
