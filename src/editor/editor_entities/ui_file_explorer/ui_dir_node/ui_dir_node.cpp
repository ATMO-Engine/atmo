#include "ui_dir_node.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entity_registry.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_file_explorer.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_file_node/ui_file_node.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>

namespace fs = std::filesystem;

namespace atmo::core::ecs::entities
{
    void UIFileExplorerDirNode::RegisterSystems(flecs::world *world)
    {
        world->observer<components::UIFileExplorerNode>("UIFileExplorerNode_remove")
            .event(flecs::OnRemove)
            .each([](flecs::entity e, components::UIFileExplorerNode &node) {
                if (node.explorer_root == 0)
                    return;

                auto root_entity = core::ecs::EntityRegistry::GetEntityFromId(node.explorer_root);
                if (!root_entity.is_alive())
                    return;

                UIFileExplorer root(root_entity);
                auto &explorer_comp = root.getComponentMutable<components::UIFileExplorer>();

                if (explorer_comp.focused_node != e)
                    return;

                explorer_comp.focused_node = {};
                explorer_comp.focused_path.clear();
            });
    }

    void UIFileExplorerDirNode::initialize()
    {
        UIFoldableTreeItem::initialize();
        setComponent<components::UIFileExplorerNode>({});

        getComponentMutable<core::components::UIFoldableTreeItem>().open = false;
    }

    void UIFileExplorerDirNode::setPath(const std::string &path, bool show_hidden, bool force_scan)
    {
        auto &node = getComponentMutable<components::UIFileExplorerNode>();
        node.full_path = path;
        node.is_directory = true;

        std::string dirname = fs::path(path).filename().string();
        if (dirname.empty())
            dirname = path;
        rename(dirname);

        UILabel(getTitleLabel()).setText(dirname);

        auto handle = p_handle;
        getTitleButton().getSignal<>("Pressed").connect([handle, show_hidden]() {
            if (!handle.is_alive())
                return;

            UIFileExplorerDirNode dirNode(core::ecs::EntityRegistry::GetEntityFromId(handle));
            auto &foldComp = dirNode.getComponentMutable<core::components::UIFoldableTreeItem>();
            foldComp.open = !foldComp.open;

            auto &node = dirNode.getComponentMutable<components::UIFileExplorerNode>();

            if (foldComp.open && !node.scanned) {
                node.scanned = true;
                dirNode.scanChildren(show_hidden);
            }

            if (node.explorer_root != flecs::entity{}) {
                UIFileExplorer root(core::ecs::EntityRegistry::GetEntityFromId(node.explorer_root));
                root.setFocus(handle, node.full_path, true);
            }
        });

        if (force_scan)
            openAndScan(show_hidden);
    }

    void UIFileExplorerDirNode::openAndScan(bool show_hidden)
    {
        auto &node = getComponentMutable<components::UIFileExplorerNode>();
        node.scanned = true;
        getComponentMutable<core::components::UIFoldableTreeItem>().open = true;
        scanChildren(show_hidden);
    }

    bool UIFileExplorerDirNode::isOpen() const
    {
        return getComponent<core::components::UIFoldableTreeItem>().open;
    }

    const std::string &UIFileExplorerDirNode::path() const
    {
        return getComponent<components::UIFileExplorerNode>().full_path;
    }

    void UIFileExplorerDirNode::scanChildren(bool show_hidden)
    {
        auto &node = getComponentMutable<components::UIFileExplorerNode>();
        UIRect container = getChildContainer();

        for (auto &child : container.getChildren()) child.destroy();

        std::vector<components::UIFileExplorerNode> entries;
        std::error_code ec;
        for (auto &it : fs::directory_iterator(node.full_path, ec)) {
            if (ec)
                break;

            std::string name = it.path().filename().string();
            if (!show_hidden && !name.empty() && name[0] == '.')
                continue;

            components::UIFileExplorerNode entry;
            entry.full_path = it.path().string();
            entry.is_directory = it.is_directory();
            entries.push_back(std::move(entry));
        }

        std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
            if (a.is_directory != b.is_directory)
                return a.is_directory > b.is_directory;
            return fs::path(a.full_path).filename() < fs::path(b.full_path).filename();
        });

        for (auto &entry : entries) {
            if (entry.is_directory) {
                auto dir = core::ecs::EntityRegistry::Create<UIFileExplorerDirNode>("Entity::UI::UIRect::UIFoldableTreeItem::UIFileExplorerDirNode");
                dir->setParent(container);
                dir->getComponentMutable<components::UIFileExplorerNode>().explorer_root = node.explorer_root;
                dir->setPath(entry.full_path, show_hidden, false);
            } else {
                auto file = core::ecs::EntityRegistry::Create<UIFileExplorerFileNode>("Entity::UI::UIRect::UIButton::UIFileExplorerFileNode");
                file->setParent(container);
                file->getComponentMutable<components::UIFileExplorerNode>().explorer_root = node.explorer_root;
                file->setPath(entry.full_path);
            }
        }
    }

    void UIFileExplorerDirNode::setHighlight(bool highlighted)
    {
        auto &rect = getTitleButton().getComponentMutable<core::components::UIRect>();
        if (highlighted) {
            rect.color.r = 0.7f;
            rect.color.g = 0.7f;
            rect.color.b = 0.7f;
            rect.color.a = 1.0f;
        } else {
            rect.color = core::types::Color::WHITE;
            rect.color.a = 0.0f;
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIFileExplorerDirNode)
ATMO_REGISTER_COMPONENT(atmo::core::components::UIFileExplorerNode)
