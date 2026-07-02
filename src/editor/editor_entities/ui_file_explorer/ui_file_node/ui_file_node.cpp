#include "ui_file_node.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entity_registry.hpp"
#include "editor/editor_entities/ui_file_explorer/ui_file_explorer.hpp"
#include "meta/auto_register.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>

namespace fs = std::filesystem;

namespace atmo::core::ecs::entities
{
    void UIFileExplorerFileNode::RegisterSystems(flecs::world *) {}

    void UIFileExplorerFileNode::initialize()
    {
        UIButton::initialize();
        setComponent<components::UIFileExplorerNode>({});

        createSignal<const std::string &>("FileSelected");

        auto handle = p_handle;
        getSignal<>("Pressed").connect([handle]() {
            if (!handle.is_alive()) {
                return;
            }
            UIFileExplorerFileNode fileNode(core::ecs::EntityRegistry::GetEntityFromId(handle));
            auto &node = fileNode.getComponentMutable<components::UIFileExplorerNode>();

            if (node.explorer_root != 0) {
                UIFileExplorer root(core::ecs::EntityRegistry::GetEntityFromId(node.explorer_root));
                root.setFocus(handle, node.full_path, false);
            }

            fileNode.getSignal<const std::string &>("FileSelected").emit(node.full_path);
        });
    }

    void UIFileExplorerFileNode::setPath(const std::string &path)
    {
        auto &node = getComponentMutable<components::UIFileExplorerNode>();
        node.full_path = path;
        node.is_directory = false;

        std::string filename = fs::path(path).filename().string();
        rename(filename);

        UILabel(getChild("Button label")).setText(filename);
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
