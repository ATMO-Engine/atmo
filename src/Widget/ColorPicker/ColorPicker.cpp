#include "ColorPicker.hpp"
#include "spdlog/spdlog.h"

ColorPicker::ColorPicker() {};

ColorPicker::~ColorPicker() {};

void ColorPicker::run()
{
    ImGui::ColorPicker4("Color", _currentColor, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::SliderFloat(" ", &_size, 0.0f, 1.0f);
    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    if (ImGui::Button("Pencil"))
        spdlog::info("pencil");
    ImGui::SameLine();
    if (ImGui::Button("Eraser"))
        spdlog::info("eraser");
    ImGui::SameLine();
    if (ImGui::Button("Bucket"))
        spdlog::info("bucket");
}

void ColorPicker::init(ColorPicker::Tool *selectedTool) { _selectedTool = selectedTool; }
