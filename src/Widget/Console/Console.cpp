#include "Console.hpp"
#include <iostream>
#include "imgui.h"
#include "spdlog/spdlog.h"

Console::Console() : sink(std::make_shared<ConsoleSink>())
{
    auto logger = spdlog::default_logger();
    logger->sinks().push_back(sink);
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
    sink->flush();
    ImGui::BeginChild("Console", ImVec2(0, 0), ImGuiChildFlags_None);
    ImGui::Text("Console - Engine running at %d FPS", (int)ImGui::GetIO().Framerate);
    ImGui::SameLine();
    textRight(fmt::format("Logs taking {:.2f} kb", sink->getFlushedLogsSize()));
    if (ImGui::Button("Clear logs"))
        sink->clearFlushedLogs();
    ImGui::BeginChild("ConsoleOutput", ImVec2(0, 0), ImGuiChildFlags_Borders);
    ImGui::SetScrollY(ImGui::GetScrollMaxY());
    for (const auto &log : sink->getFlushedLogs()) {
        ImGui::TextUnformatted(log.c_str());
    }
    ImGui::EndChild();
    ImGui::EndChild();
}

Console::ConsoleSink::ConsoleSink()
{
    set_pattern("[%Y-%m-%d %T.%e] [%^%-5l%$] %v");
    set_formatter(std::make_unique<spdlog::pattern_formatter>());
}
