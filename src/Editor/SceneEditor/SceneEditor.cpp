#include "SceneEditor.hpp"
#include "imgui.h"

SceneEditor::SceneEditor()
    : sceneHierarchy(ecs)
{
    ecs.component<Engine>();

    auto hello = ecs.entity("Hello world").set(Engine{ true });
    auto lorem = ecs.entity("Lorem ipsum").set<Engine>({ true }).child_of(hello);
    auto dolor = ecs.entity("Dolor sit amet").set<Engine>({ true }).child_of(lorem);
    auto coucou = ecs.entity("Coucou").set<Engine>({ true });
}

void SceneEditor::run()
{
    ImGui::Columns(3, "SceneEditorColumns", true);

    ImGui::SetColumnWidth(-1, ImGui::GetWindowWidth() * 0.15f);
    ImGui::BeginChild("LeftColumn", ImVec2(0, 0), false);
        ImGui::BeginChild("SceneHierarchy", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f), false);
            ImGui::Text("Scene Hierarchy");
            sceneHierarchy.run();
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::BeginChild("FileExplorer", ImVec2(0, 0), false);
            ImGui::Text("File Explorer");
        ImGui::EndChild();
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::SetColumnWidth(-1, ImGui::GetWindowWidth() * 0.7f);
    ImGui::BeginChild("MiddleColumn", ImVec2(0, 0), false);
        ImGui::BeginChild("SceneView", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.6f), false);
            ImGui::Text("Scene View");
        ImGui::EndChild();

        ImGui::Separator();

        ImGui::BeginChild("Console", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.9f), false);
            ImGui::Text("Console - Running at %d FPS", (int)ImGui::GetIO().Framerate);
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
