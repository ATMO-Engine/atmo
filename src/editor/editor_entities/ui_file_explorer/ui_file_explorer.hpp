#pragma once

#include <filesystem>
#include <flecs.h>
#include <string>
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_input/ui_text_input/ui_text_input.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_dir_node/ui_dir_node.hpp"

namespace atmo::core::components
{
    struct UIFileExplorer {
        std::filesystem::path focused_path;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIFileExplorer> {
    static constexpr const char *name = "FileExplorer";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple();
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
