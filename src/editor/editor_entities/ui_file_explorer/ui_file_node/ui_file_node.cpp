#include "ui_file_node.hpp"
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

#include <filesystem>

namespace fs = std::filesystem;

namespace atmo::core::ecs::entities
{
    void UIFileExplorerFileNode::RegisterSystems(flecs::world *) {}

    void UIFileExplorerFileNode::initialize()
    {
        UIButton::initialize();
        setComponent<components::UIFileExplorerNode>({});

        getComponentMutable<components::UIRect>().corner_radius = { 4, 4, 4, 4 };

        auto file_label = core::ecs::EntityRegistry::Create<UILabel>("Entity::UI::UILabel");
        file_label->setFontSize(11);
        file_label->getComponentMutable<components::UI>().modulate = types::Color::BLACK;
        file_label->setParent(*this);
        getComponentMutable<components::UIFileExplorerNode>().label = file_label->getHandle();

        auto handle = p_handle;
        getSignal<>("Pressed").connect([handle]() {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorerFileNode fileNode(core::ecs::EntityRegistry::GetEntityFromId(handle));
            auto &node = fileNode.getComponentMutable<components::UIFileExplorerNode>();

            auto &comp = fileNode.getComponentMutable<components::UIFileExplorerNode>();
            auto selectEvt = atmo::core::event::EventRegistry::Create<atmo::core::event::events::FileSelectedEvent>("Event::FileSelectedEvent");
            selectEvt->path = comp.full_path;
            atmo::core::event::EventRegistry::Dispatch(selectEvt);
        });
    }

    void UIFileExplorerFileNode::setPath(const std::filesystem::path &path)
    {
        auto &node = getComponentMutable<components::UIFileExplorerNode>();
        node.full_path = path;
        node.is_directory = false;

        UILabel(node.label).setText(path.filename().string());
    }

    const std::string &UIFileExplorerFileNode::path() const
    {
        return getComponent<components::UIFileExplorerNode>().full_path;
    }

    void UIFileExplorerFileNode::setHighlight(bool highlighted)
    {
        auto &rect = getComponentMutable<core::components::UIRect>();
        if (highlighted) {
            rect.color = types::Color("#DBEAFE");
        } else {
            rect.color = core::types::Color::TRANSPARENT_COL;
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIFileExplorerFileNode)
