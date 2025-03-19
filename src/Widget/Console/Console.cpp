#include "Console.hpp"
#include "imgui.h"

Console::Console()
{
    // sink = std::make_shared<ConsoleSink>();
    // logger = std::make_shared<spdlog::logger>("console", sink);
    // spdlog::set_default_logger(logger);
    // spdlog::set_level(spdlog::level::info);
}

static void textRight(const std::string &text)
{
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth = ImGui::CalcTextSize(text.c_str()).x;

    ImGui::SetCursorPosX(windowWidth - textWidth);
    ImGui::Text("%s", text.c_str());
}

void Console::run()
{
    ImGui::BeginChild("Console", ImVec2(0, 0), ImGuiChildFlags_None);
    ImGui::Text("Console - Engine running at %d FPS", (int)ImGui::GetIO().Framerate);
    ImGui::SameLine();
    textRight(std::format("Logs taking %zu mb", (size_t)0.0));
    ImGui::BeginChild("ConsoleOutput", ImVec2(0, 0), ImGuiChildFlags_Borders);
    ImGui::Text("Console output");
    ImGui::Text("Console output");
    ImGui::Text("Console output");
    ImGui::Text("Console output");
    ImGui::Text("Console output");
    ImGui::EndChild();
    ImGui::EndChild();
}
