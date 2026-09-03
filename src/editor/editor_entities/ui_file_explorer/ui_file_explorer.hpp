#pragma once

#include <flecs.h>
#include <string>
#include <vector>
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_input/ui_text_input/ui_text_input.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_dir_node/ui_dir_node.hpp"
#include "file_watcher/file_watcher.hpp"

namespace atmo::core::components
{
    struct UIFileExplorer {
        std::string root_path;
        bool show_hidden = false;

        flecs::entity focused_node = {};
        std::string focused_path;
        bool focused_is_directory = false;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIFileExplorer> {
    static constexpr const char *name = "FileExplorer";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIFileExplorer::root_path>("root_path"),
        atmo::meta::field<&atmo::core::components::UIFileExplorer::show_hidden>("show_hidden"));
};

namespace atmo::core::ecs::entities
{
    class UIFileExplorer : public EntityRegistry::Registrable<UIFileExplorer, UIRect>
    {
    public:
        using EntityRegistry::Registrable<UIFileExplorer, UIRect>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIFileExplorer";
        }

        void setFocus(flecs::entity node, const std::string &path, bool is_directory);
        bool hasFocus() const;

        Clay_ElementDeclaration buildDecl() override;
        void draw(ClaySdL3RendererData *data) override;

    private:
        void rebuild();
        flecs::entity findNodeByPath(UIFileExplorerDirNode &node, const std::string &target_path);

        UIButton getAddButton() const;
        UIButton getDeleteButton() const;
        UIButton getRenameButton() const;
        UITextInput getAddInput() const;
        UITextInput getAddFolderInput() const;
        UITextInput getRenameInput() const;
        UIRect getTreeContainer() const;
    };
} // namespace atmo::core::ecs::entities
