#include "ui_dir_node.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/file_system_event/file_selected_event.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_file_explorer.hpp"
#include "meta/auto_register.hpp"

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

        auto title_button = getTitleButton();
        title_button.getComponentMutable<components::UIRect>().corner_radius = { 4, 4, 4, 4 };

        auto title_label = core::ecs::EntityRegistry::Create<UILabel>("Entity::UI::UILabel");
        title_label->setFontSize(11);
        title_label->getComponentMutable<components::UI>().modulate = types::Color::BLACK;
        title_label->setParent(title_button);
        getComponentMutable<components::UIFileExplorerNode>().label = title_label->getHandle();

        auto handle = p_handle;
        getTitleButton().getSignal<>("Pressed").connect([handle]() {
            if (!handle.is_alive())
                return;

            UIFileExplorerDirNode dirNode(core::ecs::EntityRegistry::GetEntityFromId(handle));
            auto &foldComp = dirNode.getComponentMutable<core::components::UIFoldableTreeItem>();
            foldComp.open = !foldComp.open;

            auto &comp = dirNode.getComponentMutable<components::UIFileExplorerNode>();
            auto selectEvt = atmo::core::event::EventRegistry::Create<atmo::core::event::events::FileSelectedEvent>("Event::FileSelectedEvent");
            selectEvt->path = comp.full_path;
            atmo::core::event::EventRegistry::Dispatch(selectEvt);
        });
    }

    void UIFileExplorerDirNode::setPath(const std::filesystem::path &path, bool force_scan)
    {
        auto &node = getComponentMutable<components::UIFileExplorerNode>();
        node.full_path = path;
        node.is_directory = true;

        std::string dirname = path.filename().string();
        if (dirname.empty())
            dirname = path.string();
        UILabel(node.label).setText(dirname);
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
            rect.color = types::Color("#DBEAFE");
        } else {
            rect.color = core::types::Color::TRANSPARENT_COL;
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIFileExplorerDirNode)
ATMO_REGISTER_COMPONENT(atmo::core::components::UIFileExplorerNode)
