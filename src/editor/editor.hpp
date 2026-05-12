#pragma once

#include <memory>
#include <string_view>

#include "core/engine.hpp"
#include "editor/commands/commands.hpp"
#include "editor/menu_bar/i_platform_menu_bar.hpp"

#if !defined(ATMO_EXPORT)

#define ATMO_ASCII_ART                                                         \
    "                                                                      \n" \
    "                             %@%                                      \n" \
    "                           %  *                                       \n" \
    "                  *@@***%@@@%**                                       \n" \
    "                  @@            -%@-                                  \n" \
    "                   :*@@@@@@*       =@                                 \n" \
    "                  %@=::::*@@@@@%=:-*@                                 \n" \
    "                *@           -@@@-:                                   \n" \
    "               @=               %@*                                   \n" \
    "               @                  @%                                  \n" \
    "               @                   @*                                 \n" \
    "               @@@@-     %-:=      *@                                 \n" \
    "               @:  %@:  %%= *      -@*=                               \n" \
    "               @    :@*            =@=@@@@@-                          \n" \
    "            %@%      *@                   *@@@=                       \n" \
    "          @@       :@@                       *@@@                     \n" \
    "          @@@*==*@@@%%%%                       :@@@                   \n" \
    "             :*=   @*                             @@%                 \n" \
    "                    @%                             *@@                \n" \
    "                    %@@*                            %@=               \n" \
    "                    *@@@@@:                         :@@@@@@%--:       \n" \
    "                     @*  *@@@**    :@@**@@:         :@*       -@@@@*  \n" \
    "                    @@%     @@@@@  @ -@   %         @@            =@  \n" \
    "                   %@%:  -@@@     *@      %       *@@@@@@@@@@@@@@@@*  \n" \
    "                    :*@@@@@@    -@@@@:  :*@:=--%@@*                   \n" \
    "                          -@@@@@@           :::                       \n" \
    "                                                                      \n"


namespace atmo::editor
{
    class Editor
    {
    public:
        Editor(atmo::core::Engine &engine, const std::string &project_path);
        ~Editor() = default;

        void init();

    private:
        void registerDefaultCommands();

        atmo::core::Engine &m_engine;
        std::string_view m_project_path;
        Commands m_commands;
        std::unique_ptr<IPlatformMenuBar> m_menu_bar;
    };
} // namespace atmo::editor

#endif
