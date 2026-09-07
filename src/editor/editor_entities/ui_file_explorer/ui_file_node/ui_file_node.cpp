#include "ui_file_node.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/file_system_event/file_selected_event.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_dir_node/ui_dir_node.hpp"
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

        std::string filename = fs::path(path).filename().string();
        UILabel(getChild("Button label")).setText(filename);
    }

    const std::string &UIFileExplorerFileNode::path() const
    {
        return getComponent<components::UIFileExplorerNode>().full_path;
    }

    void UIFileExplorerFileNode::setHighlight(bool highlighted)
    {
        auto &rect = getComponentMutable<core::components::UIRect>();
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

ATMO_REGISTER_ENTITY(entities::UIFileExplorerFileNode)
