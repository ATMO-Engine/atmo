#include "SceneEditor.hpp"
#include "imgui.h"

#include <filesystem>

SceneEditor::SceneEditor()
    : world(flecs::world()), sceneHierarchy(world), fileExplorer(std::filesystem::current_path())
{
    fileExplorer.refresh();
}

void SceneEditor::run()
{
    ImGui::Columns(3, "SceneEditorColumns", true);

    ImGui::SetColumnWidth(-1, ImGui::GetWindowWidth() * 0.15f);
    ImGui::BeginChild("LeftColumn", ImVec2(0, 0), false);
        ImGui::BeginChild("SceneHierarchy", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f), false);
            ImGui::Text("Scene Hierarchy");
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::BeginChild("FileExplorer", ImVec2(0, 0), false);
            ImGui::Text("File Explorer");
            fileExplorer.run();
        ImGui::EndChild();
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::SetColumnWidth(-1, ImGui::GetWindowWidth() * 0.7f);
    ImGui::BeginChild("MiddleColumn", ImVec2(0, 0), false);
        ImGui::BeginChild("SceneView", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.7f), false);
            ImGui::Text("Scene View");
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::BeginChild("Console", ImVec2(0, 0), false);
            ImGui::Text("Console");
        ImGui::EndChild();
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::SetColumnWidth(-1, ImGui::GetWindowWidth() * 0.15f);
    ImGui::BeginChild("RightColumn", ImVec2(0, 0), false);
        ImGui::BeginChild("Inspector", ImVec2(0, 0), false);
            ImGui::Text("Inspector");
        ImGui::EndChild();
    ImGui::EndChild();

    ImGui::Columns(1);
}
