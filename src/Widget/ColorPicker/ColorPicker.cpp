#include "ColorPicker.hpp"

ColorPicker::ColorPicker(){};

ColorPicker::~ColorPicker(){};

void ColorPicker::run()
{
    ImGui::ColorPicker4("Color", _currentColor, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::SliderFloat(" ", &_size, 0.0f, 1.0f);
    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::Button("Pencil");
    ImGui::SameLine();
    ImGui::Button("Eraser");
    ImGui::SameLine();
    ImGui::Button("Bucket");
}
