#pragma once

#include <flecs.h>
#include <string>
#include "core/ecs/entities/ui/ui_foldable_tree_item/ui_foldable_tree_item.hpp"

namespace atmo::core::components
{
    struct UIFileExplorerNode {
        std::string full_path;
        bool is_directory = false;
        bool scanned = false;
        flecs::entity explorer_root = {};
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::UIFileExplorerNode> {
    static constexpr const char *name = "FileExplorerNode";
    static constexpr const char *category = "UI";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::UIFileExplorerNode::full_path>("full_path"),
        atmo::meta::field<&atmo::core::components::UIFileExplorerNode::is_directory>("is_directory"));
};

namespace atmo::core::ecs::entities
{
    class UIFileExplorerDirNode : public EntityRegistry::Registrable<UIFileExplorerDirNode, UIFoldableTreeItem>
    {
    public:
        using EntityRegistry::Registrable<UIFileExplorerDirNode, UIFoldableTreeItem>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "UIFileExplorerDirNode";
        }

        void setPath(const std::string &path, bool show_hidden, bool force_scan = false);

        void openAndScan(bool show_hidden);

        bool isOpen() const;
        const std::string &path() const;

        void scanChildren(bool show_hidden);

        void setHighlight(bool highlighted);
    };
} // namespace atmo::core::ecs::entities
