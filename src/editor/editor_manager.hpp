#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <vector>

#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_foldable_tree_item/ui_foldable_tree_item.hpp"
#include "core/ecs/entities/ui/ui_image/ui_image.hpp"
#include "core/engine.hpp"
#include "editor/child_process.hpp"
#include "editor/commands/commands.hpp"
#include "editor/editors/editor.hpp"
#include "editor/menu_bar/i_platform_menu_bar.hpp"
#include "flecs/addons/cpp/entity.hpp"
#include "luau/luau.hpp"

#include "editor_registry.hpp"

namespace atmo::project
{
    struct ProjectSettings;
} // namespace atmo::project

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

        std::shared_ptr<Editor> openEditor(const std::string &type_name, const std::optional<std::string> &path = std::nullopt);
        std::shared_ptr<Editor> openFile(const std::filesystem::path &path);
        std::shared_ptr<Editor> findEditor(std::uint32_t id) const;
        std::shared_ptr<Editor> findEditorByPath(const std::filesystem::path &path) const;
        void activateEditor(const std::shared_ptr<Editor> &editor);
        void closeEditor(const std::shared_ptr<Editor> &editor);
        std::shared_ptr<Editor> activeEditor() const;

        const std::vector<std::shared_ptr<Editor>> &editors() const
        {
            return m_editors;
        }

        Commands &commands()
        {
            return m_commands;
        }

        const std::string &projectPath() const
        {
            return m_project_path;
        }

        bool startPlay(const std::optional<std::string> &scene = std::nullopt, const std::vector<std::string> &extra_args = {});
        void stopPlay();
        bool isPlaying();

        static std::optional<std::string> EditorTypeForPath(const std::filesystem::path &path);
        static std::filesystem::path ResolveProjectPath(const std::string &path);

    private:
        void registerDefaultCommands();
        void registerShortcutDispatcher();
        void updateTopBar();
        void makeToolbar(core::ecs::entities::UI container);
        void addEditor(const std::shared_ptr<Editor> &editor);
        void updatePlayButton();

        void handleSave();
        void handleSaveAs();
        void handleOpen();

        void openProjectSettings();
        std::shared_ptr<core::ecs::entities::UIFoldableTreeItem> makeSettingsSection(core::ecs::entities::UI &body, const std::string &title);

        atmo::core::Engine &m_engine;
        std::uint32_t m_next_editor_id = 1;
        std::string m_project_path;
        Commands m_commands;
        std::unique_ptr<IPlatformMenuBar> m_menu_bar;
        std::vector<std::shared_ptr<Editor>> m_editors;
        std::vector<std::shared_ptr<core::ecs::entities::UI>> m_editor_containers;
        std::vector<std::shared_ptr<core::ecs::entities::UI>> m_editor_tool_containers;
        std::shared_ptr<core::ecs::entities::UI> m_editor_container;
        std::shared_ptr<core::ecs::entities::UIRect> m_topbar;
        std::shared_ptr<core::ecs::entities::UIRect> m_toolbar;
        std::optional<std::size_t> m_active_editor_index;
        std::weak_ptr<Editor> m_editor_before_close;
        std::shared_ptr<core::ecs::entities::UIButton> m_play_btn;
        std::shared_ptr<core::ecs::entities::UIImage> m_play_btn_icon;
        std::optional<ChildProcess> m_play_process;
        std::shared_ptr<project::ProjectSettings> m_settings_draft;
    };
} // namespace atmo::editor

#endif
