#include "SceneEditor.hpp"
#include "SDL3/SDL_video.h"
#include "imgui.h"


SceneEditor::SceneEditor(SDL_Renderer *rend) :
    selectedEntity(-1), sceneHierarchy(ecs, selectedEntity), fileExplorer(std::filesystem::current_path()),
    entityInspector(ecs, selectedEntity), _sceneView(ecs, rend)
{
    fileExplorer.refresh();
    EntityCreator::registerComponents(ecs);
}

void SceneEditor::run()
{
    ImGui::Columns(3, "SceneEditorColumns", true);

    ImGui::SetColumnWidth(-1, ImGui::GetWindowWidth() * 0.15f);
    ImGui::BeginChild("LeftColumn", ImVec2(0, 0), false);
    ImGui::BeginChild("SceneHierarchy", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f), false);
    sceneHierarchy.run();
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::BeginChild("FileExplorer", ImVec2(0, 0), false);
    fileExplorer.run();
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::SetColumnWidth(-1, ImGui::GetWindowWidth() * 0.7f);
    ImGui::BeginChild("MiddleColumn", ImVec2(0, 0), false);
    ImGui::BeginChild("SceneView", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.6f), false);
    _sceneView.run();
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::BeginChild("Console", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.9f), false);
    console.run();
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::SetColumnWidth(-1, ImGui::GetWindowWidth() * 0.15f);
    ImGui::BeginChild("RightColumn", ImVec2(0, 0), false);
    ImGui::BeginChild("Inspector", ImVec2(0, 0), false);
    entityInspector.run();
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::Columns(1);
}

void SceneEditor::init()
{
    _sceneView.init();
}
