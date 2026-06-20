#include "scene_editor.hpp"
#include <string>
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_foldable_tree_item/ui_foldable_tree_item.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_panel/ui_panel.hpp"
#include "editor/editor_registry.hpp"

namespace atmo::editor
{
    void entityComponentFodableTreeinit(flecs::entity entity, core::ecs::entities::Entity parent)
    {
        std::vector<const meta::TypeInfo *> ti_vector;

        entity.each([&](flecs::id id) {
            if (id.is_pair())
                return;

            const meta::TypeInfo *ti = meta::MetaRegistry::Instance().findByFlecsId(id.raw_id());
            if (!ti || !ti->to_json)
                return;

            const void *comp = entity.try_get(id);
            if (!comp)
                return;

            ti_vector.emplace_back(ti);
        });

        for (auto &entity_ti : ti_vector) {
            auto child_UI = core::ecs::EntityRegistry::Create<core::ecs::entities::UIFoldableTreeItem>("Entity::UI::UIRect::UIFoldableTreeItem");
            auto &child_UI_layout = child_UI->getComponentMutable<core::components::Layout>();
            auto &child_UI_rect = child_UI->getComponentMutable<core::components::UIRect>();
            auto title_button = child_UI->getTitleButton();
            auto &title_button_comp = title_button.getComponentMutable<core::components::UIButton>();
            auto title_label = child_UI->getTitleLabel();

            child_UI_rect.color.a = 0.0f;
            child_UI_layout.direction = core::components::Layout::Direction::Vertical;
            child_UI_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            child_UI_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            child_UI_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 24.0f, 0.0f };
            child_UI_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
            child_UI_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
            child_UI_layout.child_gap = 8;
            child_UI->setParent(parent);
            title_label.setText(entity_ti->name);
            spdlog::info(entity_ti->name);
        }
    }

    void SceneEditor::init(atmo::core::ecs::entities::UI &container)
    {
        auto scene_editor_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &scene_editor_container_layout = scene_editor_container->getComponentMutable<core::components::Layout>();
        scene_editor_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_editor_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_editor_container_layout.padding = { 16, 16, 8, 16 };
        scene_editor_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::End;
        scene_editor_container->setParent(container);

        auto left_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &left_panel_container_layout = left_panel_container->getComponentMutable<core::components::Layout>();
        left_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        left_panel_container_layout.width.size = 0.2f;
        left_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        left_panel_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        left_panel_container->setParent(*scene_editor_container);

        auto left_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPanel>("Entity::UI::UIRect::UIPanel");
        auto &left_panel_rect = left_panel->getComponentMutable<core::components::UIRect>();
        left_panel_rect.color = core::types::Color::WHITE;
        left_panel_rect.corner_radius = { 4, 4, 4, 4 };
        auto &left_panel_layout = left_panel->getComponentMutable<core::components::Layout>();
        left_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_layout.direction = core::components::Layout::Direction::Vertical;
        left_panel->setParent(*left_panel_container);

        auto top_left_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &top_left_panel_container_layout = top_left_panel_container->getComponentMutable<core::components::Layout>();
        top_left_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        top_left_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        top_left_panel_container_layout.height.size = 0.1f;
        top_left_panel_container_layout.padding = { 16, 16, 8, 8 };
        top_left_panel_container_layout.child_gap = 8;
        top_left_panel_container->setParent(*left_panel);

        auto left_panel_search_bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &left_panel_search_bar_rect = left_panel_search_bar->getComponentMutable<core::components::UIRect>();
        left_panel_search_bar_rect.color = core::types::Color::BLACK;
        left_panel_search_bar_rect.color.a = 0.30f;
        auto &left_panel_search_bar_layout = left_panel_search_bar->getComponentMutable<core::components::Layout>();
        left_panel_search_bar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_search_bar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        left_panel_search_bar_layout.height.size = 0.4f;
        left_panel_search_bar->setParent(*top_left_panel_container);

        auto left_panel_pin = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &left_panel_pin_rect = left_panel_pin->getComponentMutable<core::components::UIRect>();
        left_panel_pin_rect.color = core::types::Color::BLACK;
        left_panel_pin_rect.color.a = 1.0f;
        auto &left_panel_pin_layout = left_panel_pin->getComponentMutable<core::components::Layout>();
        left_panel_pin_layout.aspect_ratio = { 1, 1 };
        left_panel_pin_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_pin->setParent(*top_left_panel_container);

        auto content_left_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &content_left_panel_container_layout = content_left_panel_container->getComponentMutable<core::components::Layout>();
        content_left_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        content_left_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        content_left_panel_container_layout.direction = core::components::Layout::Direction::Vertical;
        content_left_panel_container_layout.child_gap = 8;
        content_left_panel_container_layout.padding = { 16, 16, 0, 0 };
        content_left_panel_container->setParent(*left_panel);

        auto add_node_button = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &add_node_button_rect = add_node_button->getComponentMutable<core::components::UIRect>();
        add_node_button_rect.color = core::types::Color::BLACK;
        add_node_button_rect.color.a = 0.3f;
        auto &add_node_button_layout = add_node_button->getComponentMutable<core::components::Layout>();
        add_node_button_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        add_node_button_layout.width.size = 1.0f;
        add_node_button_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        add_node_button_layout.height.size = 0.05f;
        add_node_button->setParent(*content_left_panel_container);

        auto scene_viewport_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &scene_viewport_container_layout = scene_viewport_container->getComponentMutable<core ::components::Layout>();
        scene_viewport_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_viewport_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_viewport_container_layout.direction = core::components::Layout::Direction::Vertical;
        scene_viewport_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        scene_viewport_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        scene_viewport_container_layout.padding = { 0, 0, 8, 16 };
        scene_viewport_container->setParent(*content_left_panel_container);

        auto middle_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &middle_panel_container_layout = middle_panel_container->getComponentMutable<core::components::Layout>();
        middle_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        middle_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        middle_panel_container_layout.height.size = 0.1f;
        middle_panel_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        middle_panel_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        middle_panel_container->setParent(*scene_editor_container);

        auto middle_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPanel>("Entity::UI::UIRect::UIPanel");
        auto &middle_panel_rect = middle_panel->getComponentMutable<core::components::UIRect>();
        middle_panel_rect.color = core::types::Color::WHITE;
        middle_panel_rect.corner_radius = { 4, 4, 4, 4 };
        auto &middle_panel_layout = middle_panel->getComponentMutable<core::components::Layout>();
        middle_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        middle_panel_layout.width.size = 0.3f;
        middle_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        middle_panel_layout.height.size = 0.6f;
        middle_panel->setParent(*middle_panel_container);

        auto right_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &right_panel_container_layout = right_panel_container->getComponentMutable<core::components::Layout>();
        right_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        right_panel_container_layout.width.size = 0.2f;
        right_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        right_panel_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        right_panel_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        right_panel_container->setParent(*scene_editor_container);

        auto right_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPanel>("Entity::UI::UIRect::UIPanel");
        auto &right_panel_rect = right_panel->getComponentMutable<core::components::UIRect>();
        right_panel_rect.color = core::types::Color::WHITE;
        right_panel_rect.corner_radius = { 4, 4, 4, 4 };
        auto &right_panel_layout = right_panel->getComponentMutable<core::components::Layout>();
        right_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        right_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        right_panel_layout.direction = core::components::Layout::Direction::Vertical;
        right_panel->setParent(*right_panel_container);

        auto top_right_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &top_right_panel_container_layout = top_right_panel_container->getComponentMutable<core::components::Layout>();
        top_right_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        top_right_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        top_right_panel_container_layout.height.size = 0.1f;
        top_right_panel_container_layout.direction = core::components::Layout::Direction::Horizontal;
        top_right_panel_container_layout.padding = { 16, 16, 8, 8 };
        top_right_panel_container_layout.child_gap = 8;
        top_right_panel_container->setParent(*right_panel);

        auto right_panel_pin = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &right_panel_pin_rect = right_panel_pin->getComponentMutable<core::components::UIRect>();
        right_panel_pin_rect.color = core::types::Color::BLACK;
        right_panel_pin_rect.color.a = 1.0f;
        auto &right_panel_pin_layout = right_panel_pin->getComponentMutable<core::components::Layout>();
        right_panel_pin_layout.aspect_ratio = { 1, 1 };
        right_panel_pin_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        right_panel_pin->setParent(*top_right_panel_container);

        auto right_panel_search_bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &right_panel_search_bar_rect = right_panel_search_bar->getComponentMutable<core::components::UIRect>();
        right_panel_search_bar_rect.color = core::types::Color::BLACK;
        right_panel_search_bar_rect.color.a = 0.30f;
        auto &right_panel_search_bar_layout = right_panel_search_bar->getComponentMutable<core::components::Layout>();
        right_panel_search_bar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        right_panel_search_bar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        right_panel_search_bar_layout.height.size = 0.4f;
        right_panel_search_bar->setParent(*top_right_panel_container);

        auto content_right_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &content_right_panel_container_layout = content_right_panel_container->getComponentMutable<core::components::Layout>();
        content_right_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        content_right_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        content_right_panel_container_layout.direction = core::components::Layout::Direction::Vertical;
        content_right_panel_container_layout.child_gap = 8;
        content_right_panel_container_layout.padding = { 16, 16, 0, 0 };
        content_right_panel_container->setParent(*right_panel);

        auto component_viewport_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &component_viewport_container_layout = component_viewport_container->getComponentMutable<core ::components::Layout>();
        component_viewport_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        component_viewport_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        component_viewport_container_layout.direction = core::components::Layout::Direction::Vertical;
        component_viewport_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        component_viewport_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        component_viewport_container_layout.padding = { 0, 0, 8, 16 };
        component_viewport_container->setParent(*content_right_panel_container);

        for (auto &entity : container.getScene()->getChildren()) {
            sceneEntityFodableTreeinit(entity, *scene_viewport_container, *component_viewport_container);
        }
    }

    void SceneEditor::sceneEntityFodableTreeinit(
        core::ecs::entities::Entity entity, core::ecs::entities::Entity parent, core::ecs::entities::Entity component_container)
    {

        if (entity.getComponent<atmo::core::components::EntityBase>().type_name.starts_with("Entity::Entity2d")) {
            auto child_UI = core::ecs::EntityRegistry::Create<core::ecs::entities::UIFoldableTreeItem>("Entity::UI::UIRect::UIFoldableTreeItem");
            auto &child_UI_layout = child_UI->getComponentMutable<core::components::Layout>();
            auto &child_UI_rect = child_UI->getComponentMutable<core::components::UIRect>();
            auto title_button = child_UI->getTitleButton();
            auto &title_button_comp = title_button.getComponentMutable<core::components::UIButton>();
            auto title_label = child_UI->getTitleLabel();
            auto entity_handle = entity.getHandle();
            auto title_button_handle = title_button.getHandle();

            child_UI_rect.color.a = 0.0f;
            child_UI_layout.direction = core::components::Layout::Direction::Vertical;
            child_UI_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            child_UI_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            child_UI_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 24.0f, 0.0f };
            child_UI_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
            child_UI_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
            child_UI_layout.child_gap = 8;
            child_UI->setParent(parent);
            title_label.setText(std::string(entity.name()));
            title_button_comp.toggle = true;
            title_button_comp.group = 1;
            title_button.getSignal<int>("Toggle").connect([this, entity_handle, title_button_handle, component_container](int group_id) {
                auto button = core::ecs::entities::Entity(title_button_handle);
                auto &button_comp = button.getComponentMutable<core::components::UIButton>();

                if (button_comp.is_pressed) {
                    m_selected_entity = entity_handle;
                    auto children = component_container.getChildren();

                    for (auto &child : children) child.destroy();
                    entityComponentFodableTreeinit(m_selected_entity, component_container);
                }
            });

            for (auto &child : entity.getChildren()) sceneEntityFodableTreeinit(child, child_UI->getChildContainer(), component_container);

            // if (entity.getChildren().empty()) {
            //     auto &child_container = child_UI->getChildren()[1].getComponentMutable<core::components::UI>();

            //     child_container.visible = false;
            // }
        }
    }

    flecs::entity SceneEditor::getSelectedEntity()
    {
        return m_selected_entity;
    }

    void SceneEditor::setSelectedEntity(flecs::entity new_slected_entity)
    {
        m_selected_entity = new_slected_entity;
    }
} // namespace atmo::editor

ATMO_REGISTER_EDITOR(atmo::editor::SceneEditor);
