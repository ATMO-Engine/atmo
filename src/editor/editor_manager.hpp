#pragma once

#include <memory>
#include <stack>
#include <string_view>
#include <vector>

#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/engine.hpp"
#include "editor/commands/commands.hpp"
#include "editor/editors/editor.hpp"
#include "editor/menu_bar/i_platform_menu_bar.hpp"
#include "flecs/addons/cpp/entity.hpp"
#include "luau/luau.hpp"

#include "editor_registry.hpp"

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
    class EditorManager
    {
    public:
        EditorManager(atmo::core::Engine &engine, const std::string &project_path);
        ~EditorManager() = default;

        void init();

        void openNewEditorSelectionPopup();
        core::ecs::entities::UIButton makeEditorCreationButton(const std::string &editor);
        void updateTopBar();

    private:
        void registerDefaultCommands();

        atmo::core::Engine &m_engine;
        std::string_view m_project_path;
        Commands m_commands;
        std::unique_ptr<IPlatformMenuBar> m_menu_bar;
        std::vector<std::shared_ptr<Editor>> m_editors;
        std::vector<std::shared_ptr<core::ecs::entities::UI>> m_editor_containers;
        std::vector<std::shared_ptr<core::ecs::entities::UI>> m_editor_tool_containers;
        std::shared_ptr<core::ecs::entities::UI> m_editor_container;
        std::shared_ptr<core::ecs::entities::UIRect> m_topbar;
    };
} // namespace atmo::editor

#endif
