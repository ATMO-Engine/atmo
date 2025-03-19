#include "FrameEditor.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <utility>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui.h"
#include "spdlog/spdlog.h"
#include "FrameEditor.hpp"


FrameEditor::FrameEditor() : _drawnList(std::make_pair(DrawingContext(0, 0, 0, 255, 2), std::vector<Point>())) {}

FrameEditor::~FrameEditor() {}

bool FrameEditor::init() { return true; }

void FrameEditor::draw() {}

void FrameEditor::run()
{
    ImVec2 canvasP0 = ImGui::GetCursorScreenPos(); // Top-left of the canvas
    ImVec2 canvasSize = ImGui::GetContentRegionAvail(); // Get available size
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(canvasP0, ImVec2(canvasP0.x + canvasSize.x, canvasP0.y + canvasSize.y),
                            IM_COL32(255, 255, 255, 255)); // White background


    if (ImGui::IsMouseReleased(0)) {
        _drawnPoints.push_back(_drawnList);
        _drawnList.first = DrawingContext(_r, _g, _b, _a, _thickness);
        _drawnList.second.clear();
    }
    if (ImGui::IsMouseDown(0)) { // Left mouse button
        ImVec2 mousePos = ImGui::GetMousePos();
        if (mousePos.x > canvasP0.x && mousePos.x < (canvasP0.x + canvasSize.x) && mousePos.y > canvasP0.y &&
            mousePos.y < (canvasP0.y + canvasSize.y)) {
            _drawnList.second.push_back({(int)mousePos.x, (int)mousePos.y});
        }
    }


    // Draw stored points
    for (auto &toLineUp : _drawnPoints) {
        for (size_t i = 1; i < toLineUp.second.size(); i++) {
            int col = toLineUp.first.getColor();
            float thick = toLineUp.first.getThickness();
            auto &toDraw = toLineUp.second;
            drawList->AddCircleFilled(ImVec2(toDraw[i].x, toDraw[i].y), thick, IM_COL32((col >> 24) & (0b11111111), (col >> 16) & (0b11111111), (col >> 8) & (0b11111111), col & 0b11111111));

        }
    }

    for (size_t i = 0; i < _drawnList.second.size(); i++) {
        int col = _drawnList.first.getColor();
        float thick = _drawnList.first.getThickness();
        auto &toDraw = _drawnList.second;
        drawList->AddCircleFilled(ImVec2(toDraw[i].x, toDraw[i].y), thick, IM_COL32((col >> 24) & (0b11111111), (col >> 16) & (0b11111111), (col >> 8) & (0b11111111), col & 0b11111111));
    }
}
