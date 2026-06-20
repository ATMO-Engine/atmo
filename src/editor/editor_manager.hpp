#pragma once

#include <memory>
#include <stack>
#include <string_view>
#include <vector>

#include "core/ecs/entities/entity.hpp"
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

        void
        sceneEntityFodableTreeinit(core::ecs::entities::Entity entity, core::ecs::entities::Entity parent, core::ecs::entities::Entity component_container);

        flecs::entity getSelectedEntity();
        void setSelectedEntity(flecs::entity new_slected_entity);

    private:
        void registerDefaultCommands();

        atmo::core::Engine &m_engine;
        std::string_view m_project_path;
        Commands m_commands;
        std::unique_ptr<IPlatformMenuBar> m_menu_bar;
        std::vector<std::unique_ptr<Editor>> m_editors;
        flecs::entity m_selected_entity;
    };
} // namespace atmo::editor

#endif
