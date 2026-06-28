#include "scene_editor.hpp"
#include <cmath>
#include <string>
#include <utility>
#include "SDL3/SDL_keyboard.h"
#include "core/ecs/entities/2d/physics_2d/body_2d/dynamic_2d/dynamic_2d.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/static_2d/static_2d.hpp"
#include "core/ecs/entities/2d/sprite_2d/sprite_2d.hpp"
#include "core/ecs/entities/scene/scene.hpp"
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
#include "core/ecs/world_context.hpp"
#include "core/event/event_registry.hpp"
#include "core/input/input_manager.hpp"
#include "core/resource/subresource_registry.hpp"
#include "core/resource/subresources/2d/shape/circle_shape2d.hpp"
#include "core/resource/subresources/2d/shape/rectangle_shape2d.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_panel/ui_panel.hpp"
#include "editor/editor_registry.hpp"
#include "flecs/addons/cpp/mixins/id/decl.hpp"
#include "meta/widget_registry.hpp"
#include "spdlog/spdlog.h"

namespace atmo::editor
{
    void entityComponentFodableTreeinit(flecs::entity entity, core::ecs::entities::Entity parent)
    {
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

            for (auto &field_info : entity_ti.second->fields)
                auto widget = meta::WidgetRegistry::get().create(child_UI->getChildContainer(), entity.try_get_mut(entity_ti.first), field_info);

            // auto &input_type = inputtest->getComponentMutable<core::components::UIInput>();
            // input_type.input_type = core::components::UIInput::InputType::Text;

            // inputtest->setParent(child_UI->getChildContainer());
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
            m_scene_ctx->init(renderer, 800, 600);

            core::event::EventRegistry::SetCallBack<editor::ProgressTickEvent>([ctx = m_scene_ctx.get(), handle = root](editor::ProgressTickEvent *evt) {
                SDL_Renderer *renderer = nullptr;
                if (handle.is_valid() && handle.has<core::components::Window>()) {
                    auto window = handle.get_ref<core::components::Window>();
                    if (window) {
                        renderer = window->renderer_data.renderer;
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
            });

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
            } else {
                spdlog::error("Couldn't create scene viewport");
            }

            auto rectangle_shape =
                core::resource::SubResourceRegistry::Create<core::resource::resources::RectangleShape2d>("SubResource::Shape2d::RectangleShape2d");
            rectangle_shape->setSize({ 800, 100 });

            auto static_body =
                core::ecs::EntityRegistry::CreateIn<core::ecs::entities::Static2d>(&m_scene_ctx->getWorld(), "Entity::Entity2d::Body2d::Static2d");
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


            // Sprite
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

        // auto save_button = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        // auto &save_button_rect = save_button->getComponentMutable<core::components::UIRect>();
        // save_button_rect.color = core::types::Color::BLACK;
        // save_button_rect.color.a = 0.5f;
        // auto &save_button_layout = save_button->getComponentMutable<core::components::Layout>();
        // save_button_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        // save_button_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        // save_button_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 60.0f, 60.0f };
        // save_button->setParent(*top_left_panel_container);
        // save_button->getSignal<bool>("Toggle").connect([this](bool /*state*/) {
        //     if (!m_scene_ctx || !m_scene_ctx->isReady())
        //         return;
        //     const std::string path = m_scene_file_path.empty() ? "scene.json" : m_scene_file_path;
        //     m_scene_ctx->saveSceneToFile(path);
        // });

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

        auto scene_viewport_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &scene_viewport_container_layout = scene_viewport_container->getComponentMutable<core ::components::Layout>();
        scene_viewport_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_viewport_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_viewport_container_layout.direction = core::components::Layout::Direction::Vertical;
        scene_viewport_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        scene_viewport_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        scene_viewport_container_layout.padding = { 0, 0, 8, 8 };
        scene_viewport_container->setParent(*content_left_panel_container);

        auto middle_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &middle_panel_container_layout = middle_panel_container->getComponentMutable<core::components::Layout>();
        middle_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        middle_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        middle_panel_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 36.0f, 36.0f };
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
                sceneEntityFodableTreeinit(entity, *scene_viewport_container, *component_viewport_container);
            }
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
            title_button.getSignal<bool>("Toggle").connect([this, entity_handle, title_button_handle, component_container](bool /*new_state*/) {
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
