#include "ui_dir_node.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/file_system_event/reload_explorer_event.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_file_explorer.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_file_node/ui_file_node.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace atmo::core::ecs::entities
{
    void UIFileExplorerDirNode::RegisterSystems(flecs::world *world) {}

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

            if (foldComp.open && !node.open) {
                node.open = true;
            }

            if (node.explorer_root != flecs::entity{}) {
                UIFileExplorer root(core::ecs::EntityRegistry::GetEntityFromId(node.explorer_root));
                root.setFocus(handle, node.full_path, true);
            }

            auto reload = atmo::core::event::EventRegistry::Create<atmo::core::event::events::ReloadExplorerEvent>("Event::ReloadExplorerEvent");
            atmo::core::event::EventRegistry::Dispatch(reload);
        });
    }

    bool UIFileExplorerDirNode::isOpen() const
    {
        return getComponent<core::components::UIFoldableTreeItem>().open;
    }

    const std::string &UIFileExplorerDirNode::path() const
    {
        return getComponent<components::UIFileExplorerNode>().full_path;
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
