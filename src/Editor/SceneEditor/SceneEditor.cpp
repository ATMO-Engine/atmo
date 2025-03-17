#include "SceneEditor.hpp"
#include "imgui.h"


SceneEditor::SceneEditor() :
    selectedEntity(-1), sceneHierarchy(ecs, selectedEntity), fileExplorer(std::filesystem::current_path()),
    entityInspector(ecs, selectedEntity)
{
    fileExplorer.refresh();
    EntityCreator::registerComponents(ecs);

    auto hello = EntityCreator::createEntity(ecs, "Hello world").set<Transform>({});
    auto lorem = EntityCreator::createEntity(ecs, "Lorem ipsum").set<Transform>({}).child_of(hello);
    auto dolor = EntityCreator::createEntity(ecs, "Dolor sit amet").set<Transform>({}).child_of(lorem);
    auto coucou = EntityCreator::createEntity(ecs, "Coucou").set<Transform>({});
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
    entityInspector.run();
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::Columns(1);
}
