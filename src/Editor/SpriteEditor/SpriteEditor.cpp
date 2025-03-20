#include "SpriteEditor.hpp"
#include "SDL3/SDL_render.h"
#include "imgui.h"
#include "imgui_internal.h"


SpriteEditor::SpriteEditor(SDL_Renderer *renderer) : fileExplorer(std::filesystem::current_path()), frameEditor(renderer) { fileExplorer.refresh(); }

void SpriteEditor::init()
{
    frameEditor.init(1000, 660);
    colorPicker.init(&_selectedTool);
}

void SpriteEditor::run()
{
    ImGui::Columns(2, "SpriteEditorColumns", true);

    ImGui::SetColumnWidth(-1, ImGui::GetWindowWidth() * 0.2f);
    ImGui::BeginChild("LeftColumn", ImVec2(0, 0), false);
    ImGui::BeginChild("FileExplorer",
                      ImVec2(ImGui::GetContentRegionAvail().x * 1.2f, ImGui::GetContentRegionAvail().y * 0.5f), false);
    colorPicker.run();
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::BeginChild("FileExplorer2", ImVec2(0, 0), false);
    fileExplorer.run();
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::NextColumn();

    ImGui::SetColumnWidth(-1, ImGui::GetWindowWidth() * 0.8f);
    ImGui::BeginChild("MiddleColumn", ImVec2(0, 0), false);
    ImGui::BeginChild("SceneView", ImVec2(0, ImGui::GetContentRegionAvail().y), false);
    ImGui::Text("Sprite Editor");
    frameEditor.setColor(colorPicker.getColor());
    frameEditor.setThickness(colorPicker.getSize());
    frameEditor.setEraser(colorPicker.getEraser());
    if (colorPicker.getClear()) {
        frameEditor.clear();
    }
    frameEditor.run();
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::Columns(1);
}
