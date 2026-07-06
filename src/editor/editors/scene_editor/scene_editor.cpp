#include "scene_editor.hpp"
#include <cmath>
#include <string>
#include <utility>
#include "SDL3/SDL_keyboard.h"
#include "core/ecs/entities/2d/physics_2d/body_2d/dynamic_2d/dynamic_2d.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/kinematic_2d/kinematic_2d.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/static_2d/static_2d.hpp"
#include "core/ecs/entities/2d/sprite_2d/sprite_2d.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entities/script.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_foldable_tree_item/ui_foldable_tree_item.hpp"
#include "core/ecs/entities/ui/ui_image/ui_image.hpp"
#include "core/ecs/entities/ui/ui_input/ui_number_input/ui_number_input.hpp"
#include "core/ecs/entities/ui/ui_input/ui_text_input/ui_text_input.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/progress_tick_event/progress_tick_event.hpp"
#include "core/input/input_manager.hpp"
#include "core/resource/subresource_registry.hpp"
#include "core/resource/subresources/2d/shape/circle_shape2d.hpp"
#include "core/resource/subresources/2d/shape/rectangle_shape2d.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_panel/ui_panel.hpp"
#include "editor/editor_entities/ui_popup/ui_popup.hpp"
#include "editor/editor_registry.hpp"
#include "editor/editors/editor.hpp"
#include "editor/inspector_utils.hpp"
#include "flecs/addons/cpp/mixins/id/decl.hpp"
#include "meta/widget_registry.hpp"
#include "project/file.hpp"
#include "project/file_system.hpp"
#include "spdlog/spdlog.h"

namespace atmo::editor
{
    void entityComponentFoldableTreeinit(flecs::entity entity, core::ecs::entities::Entity parent, std::vector<std::function<void()>> &update_fns)
    {
        core::ecs::entities::Entity ent(entity);
        std::vector<std::pair<flecs::id, const meta::TypeInfo *>> ti_vector;

        entity.each([&](flecs::id id) {
            if (id.is_pair())
                return;

            const meta::TypeInfo *ti = meta::MetaRegistry::Instance().findByFlecsId(id.raw_id());
            if (!ti || !ti->to_json)
                return;

            const void *comp = entity.try_get(id);
            if (!comp)
                return;

            ti_vector.emplace_back(std::make_pair(id, ti));
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
            title_label.setText(entity_ti.second->name);

            for (auto &row : buildFieldWidgetRows(entity_ti.second, entity.try_get_mut(entity_ti.first), child_UI->getChildContainer())) {
                update_fns.push_back([entity, comp_id = entity_ti.first, field_info = row.field, w = row.widget]() {
                    void *ptr = entity.try_get_mut(comp_id);
                    if (!ptr)
                        return;
                    meta::WidgetRegistry::Instance().update(w, ptr, field_info);
                });
            }
        }

        if (!ent.hasScript()) {
            auto add_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            ((core::ecs::entities::UILabel)add_btn->getChildren()[0]).setText("Add Script");

            add_btn->getSignal<>("Pressed").connect([entity, parent, &update_fns]() {
                core::ecs::entities::Entity ent(entity);
                if (!ent.isAlive()) {
                    return;
                }
                ent.addScript();

                core::SignalQueue::Enqueue([entity, parent, &update_fns]() {
                    auto children = parent.getChildren();
                    for (auto &child : children) child.destroy();
                    update_fns.clear();
                    entityComponentFoldableTreeinit(entity, parent, update_fns);
                });
            });

            add_btn->setParent(parent);
        }
    }

    void SceneEditor::init(atmo::core::ecs::entities::UI &container)
    {
        {
            flecs::entity root = container.getHandle().world().lookup("_Root");
            SDL_Renderer *renderer = nullptr;
            if (root.is_valid() && root.has<core::components::Window>()) {
                auto window = root.get_ref<core::components::Window>();
                if (window)
                    renderer = window->renderer_data.renderer;
            }
            m_scene_ctx = std::make_unique<EditorSceneContext>();
            m_scene_ctx->init(renderer);

            if (m_scene_ctx && m_scene_ctx->isReady()) {
                auto viewport_image = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
                auto &viewport_img_comp = viewport_image->getComponentMutable<core::components::UIImage>();
                auto &viewport_image_layout = viewport_image->getComponentMutable<core::components::Layout>();
                viewport_image_layout.floating = true;
                viewport_image_layout.z_index = -1;
                viewport_image_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
                viewport_image_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
                viewport_img_comp.raw_texture = m_scene_ctx->getViewportTexture();
                viewport_image->setParent(container);
                m_viewport_image = viewport_image->getHandle();
            } else {
                spdlog::error("Couldn't create scene viewport");
            }

            core::event::EventRegistry::SetCallBack<core::event::events::ProgressTickEvent>(
                [this, ctx = m_scene_ctx.get(), handle = root, vp_img = m_viewport_image](core::event::events::ProgressTickEvent *evt) {
                    SDL_Renderer *renderer = nullptr;
                    if (handle.is_valid() && handle.has<core::components::Window>()) {
                        auto window = handle.get_ref<core::components::Window>();
                        if (window) {
                            renderer = window->renderer_data.renderer;

                            if (vp_img.is_valid() && vp_img.has<core::components::UIImage>()) {
                                auto img = vp_img.get_ref<core::components::UIImage>();
                                const int w = static_cast<int>(img->rendered_size[0]);
                                const int h = static_cast<int>(img->rendered_size[1]);
                                if (w > 0 && h > 0) {
                                    ctx->resize(w, h);
                                    img->raw_texture = ctx->getViewportTexture();
                                }
                            }

                            ctx->tick(evt->delta_time, renderer);
                        }
                    }

                    auto [scroll, scroll_dt] = core::InputManager::GetScrollDelta("ui_scroll");
                    if (scroll.x != 0.0f || scroll.y != 0.0f) {
#if defined(__APPLE__)
                        const bool ctrl_held = SDL_GetModState() & SDL_KMOD_GUI;
#else
                        const bool ctrl_held = SDL_GetModState() & SDL_KMOD_CTRL;
#endif
                        if (ctrl_held) {
                            const float factor = std::pow(1.12f, scroll.y);
                            ctx->zoom(factor, { ctx->getWidth() * 0.5f, ctx->getHeight() * 0.5f });
                        } else {
                            ctx->pan({ -scroll.x * 5.0f, scroll.y * 5.0f });
                        }
                    }

                    float pinch = core::InputManager::GetPinchScale("ui_pinch");
                    if (pinch != 0.0f)
                        ctx->zoom(pinch, { ctx->getWidth() * 0.5f, ctx->getHeight() * 0.5f });

                    for (auto &fn : m_inspector_update_fns) fn();
                });

            auto circle_shape = core::resource::SubResourceRegistry::Create<core::resource::resources::CircleShape2d>("SubResource::Shape2d::CircleShape2d");
            circle_shape->setRadius(40.0f);
            circle_shape->getShapeDef().density = 2.0f;
            circle_shape->getShapeDef().material.rollingResistance = 0.02f;

            auto kinematic_body2 =
                core::ecs::EntityRegistry::CreateIn<core::ecs::entities::Kinematic2d>(&m_scene_ctx->getWorld(), "Entity::Entity2d::Body2d::Kinematic2d");
            kinematic_body2->addShape(circle_shape);
            kinematic_body2->setPosition({ 450, 0 });
            kinematic_body2->setParent(*m_scene_ctx->getScene());
            // Sprite
            auto sprite = core::ecs::EntityRegistry::CreateIn<core::ecs::entities::Sprite2d>(&m_scene_ctx->getWorld(), "Entity::Entity2d::Sprite2d");
            sprite->setTexturePath("project://assets/atmo.png");
            sprite->setParent(*kinematic_body2);
            sprite->setScale(core::types::Vector2(0.25, 0.25));
        }

        auto scene_editor_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &scene_editor_container_layout = scene_editor_container->getComponentMutable<core::components::Layout>();
        scene_editor_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_editor_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_editor_container_layout.padding = { 16, 16, 8, 16 };
        scene_editor_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::End;
        scene_editor_container->setParent(container);

        auto left_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &left_panel_container_layout = left_panel_container->getComponentMutable<core::components::Layout>();
        left_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        left_panel_container_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 320.0f, 320.0f };
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
        left_panel_layout.padding = { 16, 16, 8, 8 };
        left_panel_layout.child_gap = 8;
        left_panel->setParent(*left_panel_container);

        auto top_left_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &top_left_panel_container_layout = top_left_panel_container->getComponentMutable<core::components::Layout>();
        top_left_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        top_left_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        top_left_panel_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 30.0f, 30.0f };
        top_left_panel_container_layout.child_gap = 8;
        top_left_panel_container->setParent(*left_panel);

        auto left_panel_search_bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &left_panel_search_bar_rect = left_panel_search_bar->getComponentMutable<core::components::UIRect>();
        left_panel_search_bar_rect.color = core::types::Color::BLACK;
        left_panel_search_bar_rect.color.a = 0.30f;
        auto &left_panel_search_bar_layout = left_panel_search_bar->getComponentMutable<core::components::Layout>();
        left_panel_search_bar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_search_bar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_search_bar->setParent(*top_left_panel_container);

        auto left_panel_pin = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &left_panel_pin_rect = left_panel_pin->getComponentMutable<core::components::UIRect>();
        left_panel_pin_rect.color = core::types::Color::BLACK;
        auto &left_panel_pin_layout = left_panel_pin->getComponentMutable<core::components::Layout>();
        left_panel_pin_layout.aspect_ratio = { 1, 1 };
        left_panel_pin_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_pin_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        left_panel_pin_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 30.0f, 30.0f };
        left_panel_pin->setParent(*top_left_panel_container);

        auto content_left_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &content_left_panel_container_layout = content_left_panel_container->getComponentMutable<core::components::Layout>();
        content_left_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        content_left_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        content_left_panel_container_layout.direction = core::components::Layout::Direction::Vertical;
        content_left_panel_container_layout.child_gap = 8;
        content_left_panel_container->setParent(*left_panel);

        auto add_node_button = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &add_node_button_rect = add_node_button->getComponentMutable<core::components::UIRect>();
        add_node_button_rect.color = core::types::Color::BLACK;
        add_node_button_rect.color.a = 0.3f;
        auto &add_node_button_layout = add_node_button->getComponentMutable<core::components::Layout>();
        add_node_button_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        add_node_button_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        add_node_button_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 36.0f, 36.0f };
        add_node_button->setParent(*content_left_panel_container);

        auto window = add_node_button->getWindow()->getChildren()[0];
        add_node_button->getSignal<>("Released").connect([this, window]() { createNewEntitySelectionPopup(window); });

        auto scene_viewport_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &scene_viewport_container_layout = scene_viewport_container->getComponentMutable<core ::components::Layout>();
        scene_viewport_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_viewport_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_viewport_container_layout.direction = core::components::Layout::Direction::Vertical;
        scene_viewport_container_layout.clip.horizontal = true;
        scene_viewport_container_layout.clip.vertical = true;
        scene_viewport_container_layout.padding = { 0, 0, 8, 8 };
        scene_viewport_container->setParent(*content_left_panel_container);

        auto middle_panel_spacer = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &middle_panel_spacer_layout = middle_panel_spacer->getComponentMutable<core::components::Layout>();
        middle_panel_spacer_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        middle_panel_spacer_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        middle_panel_spacer_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        middle_panel_spacer->setParent(*scene_editor_container);

        auto right_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &right_panel_container_layout = right_panel_container->getComponentMutable<core::components::Layout>();
        right_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        right_panel_container_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 320.0f, 320.0f };
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
        right_panel_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 0.0f, 320.0f };
        right_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        right_panel_layout.direction = core::components::Layout::Direction::Vertical;
        right_panel_layout.padding = { 16, 16, 8, 8 };
        right_panel_layout.child_gap = 8;
        right_panel->setParent(*right_panel_container);

        auto top_right_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &top_right_panel_container_layout = top_right_panel_container->getComponentMutable<core::components::Layout>();
        top_right_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        top_right_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        top_right_panel_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 30.0f, 30.0f };
        top_right_panel_container_layout.child_gap = 8;
        top_right_panel_container->setParent(*right_panel);

        auto right_panel_search_bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &right_panel_search_bar_rect = right_panel_search_bar->getComponentMutable<core::components::UIRect>();
        right_panel_search_bar_rect.color = core::types::Color::BLACK;
        right_panel_search_bar_rect.color.a = 0.30f;
        auto &right_panel_search_bar_layout = right_panel_search_bar->getComponentMutable<core::components::Layout>();
        right_panel_search_bar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        right_panel_search_bar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        right_panel_search_bar->setParent(*top_right_panel_container);

        auto right_panel_pin = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &right_panel_pin_rect = right_panel_pin->getComponentMutable<core::components::UIRect>();
        right_panel_pin_rect.color = core::types::Color::BLACK;
        auto &right_panel_pin_layout = right_panel_pin->getComponentMutable<core::components::Layout>();
        right_panel_pin_layout.aspect_ratio = { 1, 1 };
        right_panel_pin_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        right_panel_pin_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        right_panel_pin_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 30.0f, 30.0f };
        right_panel_pin->setParent(*top_right_panel_container);

        auto content_right_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &content_right_panel_container_layout = content_right_panel_container->getComponentMutable<core::components::Layout>();
        content_right_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        content_right_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        content_right_panel_container_layout.direction = core::components::Layout::Direction::Vertical;
        content_right_panel_container_layout.child_gap = 8;
        content_right_panel_container->setParent(*right_panel);

        auto component_viewport_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &component_viewport_container_layout = component_viewport_container->getComponentMutable<core ::components::Layout>();
        component_viewport_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        component_viewport_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        component_viewport_container_layout.direction = core::components::Layout::Direction::Vertical;
        component_viewport_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        component_viewport_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        component_viewport_container_layout.padding = { 0, 0, 8, 8 };
        component_viewport_container->setParent(*content_right_panel_container);

        if (m_scene_ctx && m_scene_ctx->isReady() && m_scene_ctx->getScene()) {
            for (auto &entity : m_scene_ctx->getScene()->getChildren()) {
                sceneEntityFoldableTreeinit(entity, *scene_viewport_container, *component_viewport_container);
            }
        }

        m_scene_ctx->getScene()
            ->getSignal<core::ecs::entities::Entity>("child_added")
            .connect([this, scene_viewport_container, component_viewport_container](core::ecs::entities::Entity entity) {
                sceneEntityFoldableTreeinit(entity, *scene_viewport_container, *component_viewport_container);
            });
    }

    void SceneEditor::createTools()
    {
        p_tools = { Editor::EditorTool{ .type = Editor::EditorTool::Type::TOGGLE_GROUP, .name = "select", .icon_path = "project://assets/icons/x.svg" },
                    Editor::EditorTool{ .type = Editor::EditorTool::Type::TOGGLE_GROUP, .name = "measure", .icon_path = "project://assets/icons/x.svg" } };
    }

    void SceneEditor::createDemoEntities()
    {
        spdlog::debug("Making demo entities.");

        auto rectangle_shape =
            core::resource::SubResourceRegistry::Create<core::resource::resources::RectangleShape2d>("SubResource::Shape2d::RectangleShape2d");
        rectangle_shape->setSize({ 800, 100 });

        auto static_body = core::ecs::EntityRegistry::CreateIn<core::ecs::entities::Static2d>(&m_scene_ctx->getWorld(), "Entity::Entity2d::Body2d::Static2d");
        static_body->addShape(rectangle_shape);
        static_body->setPosition({ 800, 500 });
        static_body->setParent(*m_scene_ctx->getScene());

        auto rectangle_shape2 =
            core::resource::SubResourceRegistry::Create<core::resource::resources::RectangleShape2d>("SubResource::Shape2d::RectangleShape2d");
        rectangle_shape2->setSize({ 80, 80 });

        auto dynamic_body =
            core::ecs::EntityRegistry::CreateIn<core::ecs::entities::Dynamic2d>(&m_scene_ctx->getWorld(), "Entity::Entity2d::Body2d::Dynamic2d");
        dynamic_body->addShape(rectangle_shape2);
        dynamic_body->setPosition({ 410, 300 });
        dynamic_body->setParent(*m_scene_ctx->getScene());

        auto circle_shape = core::resource::SubResourceRegistry::Create<core::resource::resources::CircleShape2d>("SubResource::Shape2d::CircleShape2d");
        circle_shape->setRadius(40.0f);
        circle_shape->getShapeDef().density = 2.0f;
        circle_shape->getShapeDef().material.rollingResistance = 0.02f;

        auto dynamic_body2 =
            core::ecs::EntityRegistry::CreateIn<core::ecs::entities::Dynamic2d>(&m_scene_ctx->getWorld(), "Entity::Entity2d::Body2d::Dynamic2d");
        dynamic_body2->addShape(circle_shape);
        dynamic_body2->setPosition({ 450, 0 });
        dynamic_body2->setParent(*m_scene_ctx->getScene());


        auto sprite = core::ecs::EntityRegistry::CreateIn<core::ecs::entities::Sprite2d>(&m_scene_ctx->getWorld(), "Entity::Entity2d::Sprite2d");
        sprite->setTexturePath("project://assets/atmo.png");
        // sprite->setPosition({ 1200, 500 });
        sprite->setParent(*dynamic_body2);
        sprite->setScale(core::types::Vector2(0.25, 0.25));

        auto sprite2 = core::ecs::EntityRegistry::CreateIn<core::ecs::entities::Sprite2d>(&m_scene_ctx->getWorld(), "Entity::Entity2d::Sprite2d");
        sprite2->setTexturePath("project://assets/atmo.png");
        // sprite->setPosition({ 1200, 500 });
        sprite2->setParent(*dynamic_body2);
        sprite2->setScale(core::types::Vector2(0.25, 0.25));

        auto sprite3 = core::ecs::EntityRegistry::CreateIn<core::ecs::entities::Sprite2d>(&m_scene_ctx->getWorld(), "Entity::Entity2d::Sprite2d");
        sprite3->setTexturePath("project://assets/atmo.png");
        // sprite->setPosition({ 1200, 500 });
        sprite3->setParent(*sprite2);
        sprite3->setScale(core::types::Vector2(0.25, 0.25));

        auto sprite4 = core::ecs::EntityRegistry::CreateIn<core::ecs::entities::Sprite2d>(&m_scene_ctx->getWorld(), "Entity::Entity2d::Sprite2d");
        sprite4->setTexturePath("project://assets/atmo.png");
        // sprite->setPosition({ 1200, 500 });
        sprite4->setParent(*dynamic_body2);
        sprite4->setScale(core::types::Vector2(0.25, 0.25));

        auto sprite5 = core::ecs::EntityRegistry::CreateIn<core::ecs::entities::Sprite2d>(&m_scene_ctx->getWorld(), "Entity::Entity2d::Sprite2d");
        sprite5->setTexturePath("project://assets/atmo.png");
        // sprite->setPosition({ 1200, 500 });
        sprite5->setParent(*sprite3);
        sprite5->setScale(core::types::Vector2(0.25, 0.25));
    }

    void SceneEditor::save()
    {
        project::File scene_file = project::FileSystem::OpenFile(*p_file_path, std::ios::out);
        auto scene_data = glz::write<glz::opts{ .prettify = true }>(m_scene_ctx->getScene()->serialize()).value();
        scene_file.write(scene_data.c_str(), scene_data.size());
    }

    void SceneEditor::load()
    {
        if (!p_file_path)
            return;

        project::File scene_file = project::FileSystem::OpenFile(*p_file_path, std::ios::in);
        m_scene_ctx->loadSceneFromJson(scene_file.readAll());
    }

    void SceneEditor::sceneEntityFoldableTreeinit(
        core::ecs::entities::Entity entity, core::ecs::entities::Entity parent, core::ecs::entities::Entity component_container)
    {

        auto child_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto child_container_layout = child_container->getComponentMutable<core::components::Layout>();
        auto child_UI = core::ecs::EntityRegistry::Create<core::ecs::entities::UIFoldableTreeItem>("Entity::UI::UIRect::UIFoldableTreeItem");
        auto &child_UI_layout = child_UI->getComponentMutable<core::components::Layout>();
        auto &child_UI_rect = child_UI->getComponentMutable<core::components::UIRect>();
        auto title_button = child_UI->getTitleButton();
        auto &title_button_comp = title_button.getComponentMutable<core::components::UIButton>();
        auto title_label = child_UI->getTitleLabel();
        auto entity_handle = entity.getHandle();
        auto title_button_handle = title_button.getHandle();

        child_UI_rect.color.a = 0.0f;
        child_container_layout.direction = core::components::Layout::Direction::Vertical;
        child_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        child_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        child_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 24.0f, 0.0f };
        child_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        child_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        child_container_layout.child_gap = 8;
        child_UI->setParent(*child_container);
        child_container->setParent(parent);
        title_label.setText(std::string(entity.name()));
        title_button_comp.toggle = true;
        title_button_comp.group = 1;

        auto close_create_entity_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &close_create_entity_btn_rect = close_create_entity_btn->getComponentMutable<core::components::UIRect>();
        close_create_entity_btn_rect.color = core::types::Color::RED;
        auto &close_create_entity_btn_layout = close_create_entity_btn->getComponentMutable<core::components::Layout>();
        close_create_entity_btn->getComponentMutable<core::components::UI>().visible = false;
        close_create_entity_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        close_create_entity_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        close_create_entity_btn_layout.aspect_ratio = { 1.0f, 1.0f };
        close_create_entity_btn->getChildren()[0].destroy();
        close_create_entity_btn->setParent(*child_container);

        title_button.getSignal<bool>("Toggle").connect([this, entity_handle, title_button_handle, component_container, child_container](bool state) {
            if (state) {
                child_container->getChildren()[1].getComponentMutable<core::components::UI>().visible = state;

                m_selected_entity = entity_handle;
                auto children = component_container.getChildren();

                for (auto &child : children) child.destroy();
                m_inspector_update_fns.clear();
                entityComponentFoldableTreeinit(m_selected_entity, component_container, m_inspector_update_fns);
            }
        });

        for (auto &child : entity.getChildren()) sceneEntityFoldableTreeinit(child, child_UI->getChildContainer(), component_container);

        entity.getSignal<core::ecs::entities::Entity>("child_added").connect([this, child_UI, component_container](core::ecs::entities::Entity child) {
            sceneEntityFoldableTreeinit(child, child_UI->getChildContainer(), component_container);
        });

        entity.getParent()
            .getSignal<core::ecs::entities::Entity>("child_removed")
            .connect([this, child_container, entity_handle, component_container](core::ecs::entities::Entity removed_child) {
                if (removed_child.getHandle() != entity_handle)
                    return;

                if (m_selected_entity == entity_handle) {
                    for (auto &child : component_container.getChildren()) child.destroy();
                    m_inspector_update_fns.clear();
                    m_selected_entity = flecs::entity();
                }

                child_container->destroy();
            });

        // if (entity.getChildren().empty()) {
        //     auto &child_container = child_UI->getChildren()[1].getComponentMutable<core::components::UI>();

        close_create_entity_btn->getSignal<>("Released").connect([entity]() mutable { core::SignalQueue::Enqueue([entity]() mutable { entity.destroy(); }); });
    };


    void SceneEditor::createNewEntitySelectionPopup(core::ecs::entities::Entity parent)
    {
        auto create_entity_popup = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPopup>("Entity::UI::UIRect::UIPopup");
        auto create_entity_bg = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        create_entity_bg->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        create_entity_bg->getComponentMutable<core::components::Layout>().width.size = 0.35f;
        create_entity_bg->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        create_entity_bg->getComponentMutable<core::components::Layout>().height.size = 0.75f;
        create_entity_bg->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Vertical;
        create_entity_bg->getComponentMutable<core::components::Layout>().padding = { 8, 8, 8, 8 };
        create_entity_bg->getComponentMutable<core::components::Layout>().child_gap = 8;
        create_entity_bg->getComponentMutable<core::components::UIRect>().color = core::types::Color("#9f9f9f");
        create_entity_bg->setParent(*create_entity_popup);
        create_entity_popup->setParent(parent);

        auto create_entity_top_bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        create_entity_top_bar->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Horizontal;
        create_entity_top_bar->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        create_entity_top_bar->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        create_entity_top_bar->getComponentMutable<core::components::Layout>().height.size = core::components::Layout::SizingAxis::MinMax{ 32.0f, 32.0f };
        create_entity_top_bar->setParent(*create_entity_bg);

        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        label->setText("atmo.create_new_entity");
        label->setFontSize(24);
        label->setParent(*create_entity_top_bar);

        auto close_btn_holder = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        close_btn_holder->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_btn_holder->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_btn_holder->getComponentMutable<core::components::Layout>().child_alignment.horizontal = core::components::Layout::ChildAlignment::End;
        close_btn_holder->setParent(*create_entity_top_bar);
        auto close_create_entity_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &close_create_entity_btn_rect = close_create_entity_btn->getComponentMutable<core::components::UIRect>();
        close_create_entity_btn_rect.color = core::types::Color::RED;
        auto &close_create_entity_btn_layout = close_create_entity_btn->getComponentMutable<core::components::Layout>();
        close_create_entity_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_create_entity_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_create_entity_btn_layout.aspect_ratio = { 1.0f, 1.0f };
        close_create_entity_btn->getChildren()[0].destroy();
        close_create_entity_btn->setParent(*close_btn_holder);
        close_create_entity_btn->getSignal<>("Released").connect([create_entity_popup]() { create_entity_popup->destroy(); });

        auto entity_creation_button_list = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        entity_creation_button_list->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Vertical;
        entity_creation_button_list->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        entity_creation_button_list->getComponentMutable<core::components::Layout>().width.size = 1.0f;
        entity_creation_button_list->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        entity_creation_button_list->getComponentMutable<core::components::Layout>().child_gap = 8;
        entity_creation_button_list->getComponentMutable<core::components::Layout>().clip.vertical = true;
        entity_creation_button_list->getComponentMutable<core::components::Layout>().clip.horizontal = true;
        entity_creation_button_list->setParent(*create_entity_bg);

        auto tree = atmo::core::ecs::EntityRegistry::GetEntriesTree();

        std::function<void(core::ecs::EntityRegistry::EntityTree &, core::ecs::entities::Entity &)> buildTreeUI;
        buildTreeUI = [&](core::ecs::EntityRegistry::EntityTree &node, core::ecs::entities::Entity &parentUI) {
            size_t pos = node.entity_name.find_last_of("::");
            std::string label_name = (pos == std::string::npos) ? node.entity_name : node.entity_name.substr(pos + 1);

            if (node.entity_child.empty()) {
                auto button = makeEntityCreationButton(node.entity_name);
                button.getSignal<>("Released").connect([create_entity_popup]() { create_entity_popup->destroy(); });
                button.setParent(parentUI);
                return;
            }

            auto foldable = core::ecs::EntityRegistry::Create<core::ecs::entities::UIFoldableTreeItem>("Entity::UI::UIRect::UIFoldableTreeItem");

            foldable->getTitleLabel().setText(label_name);
            foldable->setParent(parentUI);

            if (!core::ecs::EntityRegistry::IsAbstract(node.entity_name)) {
                foldable->getTitleButton().getSignal<>("Released").connect([this, create_entity_popup, entity = node.entity_name]() {
                    auto created = core::ecs::EntityRegistry::CreateIn(&m_scene_ctx->getWorld(), entity);

                    created->setParent(*m_scene_ctx->getScene());

                    create_entity_popup->destroy();
                });
            }

            auto childContainer = foldable->getChildContainer();

            for (auto &child : node.entity_child) buildTreeUI(child, childContainer);
        };

        buildTreeUI(tree, *entity_creation_button_list);
    }

    core::ecs::entities::UIButton SceneEditor::makeEntityCreationButton(const std::string &entity_id)
    {
        auto create_entity_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &open_editor_btn_rect = create_entity_btn->getComponentMutable<core::components::UIRect>();
        auto &create_entity_btn_layout = create_entity_btn->getComponentMutable<core::components::Layout>();
        create_entity_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        create_entity_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        create_entity_btn_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 20.0f, 0.0f };
        create_entity_btn_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 20.0f, 0.0f };
        create_entity_btn_layout.padding = { 8, 8, 8, 8 };
        create_entity_btn_layout.child_gap = 8;
        create_entity_btn_layout.direction = core::components::Layout::Direction::Vertical;
        create_entity_btn->getChildren()[0].destroy();
        create_entity_btn->getSignal<>("Released").connect([this, entity_id]() {
            auto entity = core::ecs::EntityRegistry::CreateIn(&m_scene_ctx->getWorld(), entity_id);
            entity->setParent(*m_scene_ctx->getScene());
        });

        auto create_entity_topbar = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        create_entity_topbar->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        create_entity_topbar->getComponentMutable<core::components::Layout>().child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        create_entity_topbar->getComponentMutable<core::components::Layout>().child_gap = 8;
        create_entity_topbar->setParent(*create_entity_btn);

        size_t pos = entity_id.find_last_of("::");
        std::string label_name = (pos == std::string::npos) ? entity_id : entity_id.substr(pos + 1);

        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        label->setText(label_name);
        label->setFontSize(11);
        label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        label->setParent(*create_entity_topbar);

        return *create_entity_btn;
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
