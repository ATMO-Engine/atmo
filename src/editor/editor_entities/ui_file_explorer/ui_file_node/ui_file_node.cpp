#include "ui_file_node.hpp"
#include "clay.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
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
        auto &rect = getComponentMutable<components::UIRect>();
        auto file_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        node.full_path = path;
        node.is_directory = false;

        std::string filename = fs::path(path).filename().string();
        rename(filename);

        rect.corner_radius = { 4, 4, 4, 4 };
        file_label->setText(filename);
        file_label->setFontSize(11);
        file_label->getComponentMutable<components::UI>().modulate = types::Color::BLACK;
        file_label->setParent(*this);
    }

    void UIFileExplorerFileNode::setHighlight(bool highlighted)
    {
        auto &rect = getComponentMutable<core::components::UIRect>();
        if (highlighted) {
            rect.color = types::Color("#DBEAFE");
        } else {
            rect.color = core::types::Color::TRANSPARENT;
        }
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::UIFileExplorerFileNode)
