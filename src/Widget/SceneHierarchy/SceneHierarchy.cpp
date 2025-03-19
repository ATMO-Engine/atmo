#include "SceneHierarchy.hpp"
#include <cfloat>
#include "imgui.h"
#include "imgui_stdlib.h"
#include "spdlog/spdlog.h"

SceneHierarchy::SceneHierarchy(flecs::world &ecs, flecs::entity_t &selectedEntity) :
    ecs(ecs), query(ecs.query_builder<Node>().cached().build()), selectedEntity(selectedEntity)
{
}

void SceneHierarchy::logEntity(flecs::entity e, unsigned int depth)
{
    const std::string name = std::format("{}[{}] {}", std::string(2 * depth, ' '), e.id(), e.name().c_str());
    if (ImGui::Selectable(name.c_str(), selectedEntity == e.id()))
        selectedEntity = e.id();
    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        ImGui::SetTooltip("ID: %lld\nName: %s", e.id(), e.name().c_str());
    }
    if (ImGui::BeginPopupContextItem()) {
        rightClickedEntity = e.id();
        if (ImGui::MenuItem("Rename")) {
            renameBuffer = e.name();
            ImGui::CloseCurrentPopup();
            openRenamePopup = true;
        }
        if (ImGui::MenuItem("Create Child")) {
            newEntityParent = e.id();
            ImGui::CloseCurrentPopup();
            openCreateEntityPopup = true;
        }
        if (ImGui::MenuItem("Delete")) {
            deleteQueue.enqueue(e.id());
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    e.children([&](flecs::entity child) { logEntity(child, depth + 1); });
}

void SceneHierarchy::renameEntity()
{
    if (openRenamePopup) {
        ImGui::OpenPopup("RenameEntity");
        openRenamePopup = false;
    }
    if (ImGui::BeginPopupModal("RenameEntity", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoTitleBar)) {
        ImGui::Text("Rename entity %lld", rightClickedEntity);
        ImGui::InputText("##RenameEntity", &renameBuffer);
        if (ImGui::Button("OK")) {
            auto entity = ecs.entity(rightClickedEntity);
            if (!entity.is_valid() || !entity.is_alive()) {
                spdlog::warn("Entity {} is not valid or not alive", rightClickedEntity);
                renameBuffer.clear();
                ImGui::CloseCurrentPopup();
            }
            else if (renameBuffer.empty()) {
                spdlog::warn("Entity name cannot be empty");
                renameBuffer.clear();
                ImGui::CloseCurrentPopup();
            }
            else {
                entity.set_name(renameBuffer.c_str());
                renameBuffer.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            renameBuffer.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void SceneHierarchy::createEntity()
{
    if (openCreateEntityPopup) {
        ImGui::OpenPopup("CreateEntity");
        openCreateEntityPopup = false;
    }
    static bool firstFocus = true;
    if (ImGui::BeginPopupModal("CreateEntity", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoTitleBar)) {
        ImGui::Text("New entity name");
        if (firstFocus) {
            ImGui::SetKeyboardFocusHere(0);
            firstFocus = false;
        }
        if (ImGui::InputText("##CreateEntity", &renameBuffer, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::Button("OK")) {
            if (renameBuffer.size() <= 0)
                spdlog::warn("Entity name cannot be empty");
            else if (newEntityParent == -1) {
                auto entity = EntityCreator::createEntity(ecs, renameBuffer);
                if (entity.is_valid() && entity.is_alive()) {
                    spdlog::info("Created entity {} with ID {}", renameBuffer, entity.id());
                }
                else {
                    spdlog::warn("Failed to create entity");
                }
                renameBuffer.clear();
                ImGui::CloseCurrentPopup();
            }
            else {
                auto entity = EntityCreator::createEntity(ecs, renameBuffer).child_of(newEntityParent);
                if (entity.is_valid() && entity.is_alive()) {
                    spdlog::info("Created entity {} with ID {}", renameBuffer, entity.id());
                }
                else {
                    spdlog::warn("Failed to create entity");
                }
                renameBuffer.clear();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            renameBuffer.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    } else
        firstFocus = true;
}

void SceneHierarchy::run()
{
    ImGui::Text("Scene Hierarchy");
    ImGui::SameLine();
    if (ImGui::Button("+")) {
        newEntityParent = -1;
        openCreateEntityPopup = true;
    }

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
    renameEntity();
    createEntity();
}
