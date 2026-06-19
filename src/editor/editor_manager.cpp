#include "editor_manager.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_checkbox/ui_checkbox.hpp"
#include "core/ecs/entities/ui/ui_foldable_tree_item/ui_foldable_tree_item.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_panel/ui_panel.hpp"
#include "editor/editor_entities/ui_popup/ui_popup.hpp"
#include "glaze/json/prettify.hpp"
#include "locale/locale_manager.hpp"
#include "project/file_system.hpp"
#include "spdlog/spdlog.h"

#if !defined(ATMO_EXPORT)

#include "SDL3/SDL_keycode.h"
#include "core/ecs/ecs.hpp"
#include "core/ecs/entities/entity.hpp"

#ifdef __APPLE__
static constexpr SDL_Keymod PRIMARY_MOD = SDL_KMOD_GUI;
#else
static constexpr SDL_Keymod PRIMARY_MOD = SDL_KMOD_CTRL;
#endif

namespace atmo::editor
{
    EditorManager::EditorManager(atmo::core::Engine &engine, const std::string &project_path) : m_engine(engine), m_project_path(project_path) {}

    void EditorManager::registerDefaultCommands()
    {
        m_commands.registerCommand(
            {
                .id = "atmo.commands.file.save",
                .category = "atmo.commands.file.category",
                .shortcut = Shortcut{ SDLK_S, PRIMARY_MOD },
                .action = [] {},
            });

        m_commands.registerCommand(
            {
                .id = "atmo.commands.file.quit",
                .category = "atmo.commands.file.category",
                .shortcut = Shortcut{ SDLK_Q, PRIMARY_MOD },
                .action = [this] { m_engine.stop(); },
            });

        m_commands.registerCommand(
            {
                .id = "atmo.commands.edit.undo",
                .category = "atmo.commands.edit.category",
                .shortcut = Shortcut{ SDLK_Z, PRIMARY_MOD },
                .action = [] {},
            });

        m_commands.registerCommand(
            {
                .id = "atmo.commands.edit.redo",
                .category = "atmo.commands.edit.category",
                .shortcut = Shortcut{ SDLK_Z, static_cast<SDL_Keymod>(PRIMARY_MOD | SDL_KMOD_SHIFT) },
                .action = [] {},
            });
    }

    void fodableTreeinit(core::ecs::entities::Entity entity, core::ecs::entities::Entity parent)
    {

        if (entity.getComponent<atmo::core::components::EntityBase>().type_name.starts_with("Entity::Entity2d")) {
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
            title_label.setText(entity.getComponent<atmo::core::components::EntityBase>().type_name);
            title_button_comp.toggle = true;
            title_button_comp.group = 1;

            for (auto &child : entity.getChildren()) fodableTreeinit(child, child_UI->getChildContainer());

            // if (entity.getChildren().empty()) {
            //     auto &child_container = child_UI->getChildren()[1].getComponentMutable<core::components::UI>();

            //     child_container.visible = false;
            // }
        }
    }

    void EditorManager::init()
    {
        registerDefaultCommands();

        auto scene = m_engine.getECS().getCurrentScene();

        m_menu_bar = makePlatformMenuBar();
        m_menu_bar->build(*scene, m_commands);

        auto window_ui_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &window_ui_container_rect = window_ui_container->getComponentMutable<core::components::UIRect>();
        window_ui_container_rect.color.a = 0.0f;
        auto &window_ui_container_layout = window_ui_container->getComponentMutable<core::components::Layout>();
        window_ui_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        window_ui_container_layout.width.size = 1.0f;
        window_ui_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        window_ui_container_layout.height.size = 1.0f;
        window_ui_container_layout.direction = core::components::Layout::Direction::Vertical;
        window_ui_container->rename("window ui container");
        window_ui_container->setParent(*scene);

        auto topbar_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &topbar_container_rect = topbar_container->getComponentMutable<core::components::UIRect>();
        topbar_container_rect.color.a = 0.0f;
        auto &topbar_container_layout = topbar_container->getComponentMutable<core::components::Layout>();
        topbar_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        topbar_container_layout.width.size = 1.0f;
        topbar_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        topbar_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 50.0f, 50.0f };
        topbar_container_layout.direction = core::components::Layout::Direction::Horizontal;
        topbar_container_layout.padding = { 8, 8, 8, 8 };
        topbar_container->rename("topbar container");
        topbar_container->setParent(*window_ui_container);

        auto topbar = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPanel>("Entity::UI::UIRect::UIPanel");
        auto &topbar_rect = topbar->getComponentMutable<core::components::UIRect>();
        topbar_rect.color = core::types::Color::WHITE;
        auto &topbar_layout = topbar->getComponentMutable<core::components::Layout>();
        topbar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        topbar_layout.width.size = 1.0f;
        topbar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        topbar_layout.height.size = 1.0f;
        topbar_layout.padding = { 4, 4, 4, 4 };
        topbar->rename("topbar");
        topbar->setParent(*topbar_container);

        auto open_editor_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &open_editor_btn_rect = open_editor_btn->getComponentMutable<core::components::UIRect>();
        open_editor_btn_rect.color = core::types::Color::BLACK;
        auto &open_editor_btn_layout = open_editor_btn->getComponentMutable<core::components::Layout>();
        open_editor_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        open_editor_btn_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 26.0f, 26.0f };
        open_editor_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        open_editor_btn_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 26.0f, 26.0f };
        open_editor_btn->getChildren()[0].destroy();
        open_editor_btn->setParent(*topbar);
        open_editor_btn->getSignal<>("Released").connect([this]() { openNewEditorSelectionPopup(); });


        auto scene_ui_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &scene_ui_container_rect = scene_ui_container->getComponentMutable<core::components::UIRect>();
        scene_ui_container_rect.color.a = 0.0f;
        auto &scene_ui_container_layout = scene_ui_container->getComponentMutable<core::components::Layout>();
        scene_ui_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        scene_ui_container_layout.width.size = 1.0f;
        scene_ui_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        scene_ui_container_layout.height.size = 0.9f;
        scene_ui_container_layout.direction = core::components::Layout::Direction::Horizontal;
        scene_ui_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        scene_ui_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::End;
        scene_ui_container->rename("scene ui container");
        scene_ui_container->setParent(*window_ui_container);

        auto left_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &left_panel_container_rect = left_panel_container->getComponentMutable<core::components::UIRect>();
        left_panel_container_rect.color.a = 0.0f;
        auto &left_panel_container_layout = left_panel_container->getComponentMutable<core::components::Layout>();
        left_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        left_panel_container_layout.width.size = 0.2f;
        left_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_container_layout.direction = core::components::Layout::Direction::Horizontal;
        left_panel_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        left_panel_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        left_panel_container_layout.padding = { 16, 0, 8, 16 };
        left_panel_container->rename("left panel container");
        left_panel_container->setParent(*scene_ui_container);


        auto left_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPanel>("Entity::UI::UIRect::UIPanel");
        auto &left_panel_rect = left_panel->getComponentMutable<core::components::UIRect>();
        left_panel_rect.color = core::types::Color::WHITE;
        left_panel_container_rect.corner_radius = { 4, 4, 4, 4 };
        auto &left_panel_layout = left_panel->getComponentMutable<core::components::Layout>();
        left_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_layout.direction = core::components::Layout::Direction::Vertical;
        left_panel->rename("left panel");
        left_panel->setParent(*left_panel_container);
        left_panel->getSignal<core::ecs::entities::UIPanel &>("Open").connect([left_panel](core::ecs::entities::UIPanel &window) {
            left_panel->getComponentMutable<core::components::UIPanelState>().open = true;
            auto parent = window.getParent<core::ecs::entities::UIRect>();
            auto &parent_layout = parent.getComponentMutable<core::components::Layout>();
            parent_layout.padding.left += 100;
        });
        left_panel->getSignal<core::ecs::entities::UIPanel &>("Close").connect([left_panel](core::ecs::entities::UIPanel &window) {
            left_panel->getComponentMutable<core::components::UIPanelState>().open = false;
            auto parent = window.getParent<core::ecs::entities::UIRect>();
            auto &parent_layout = parent.getComponentMutable<core::components::Layout>();
            parent_layout.padding.left -= 100;
        });

        auto top_left_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &top_left_panel_container_rect = top_left_panel_container->getComponentMutable<core::components::UIRect>();
        top_left_panel_container_rect.color.a = 0.0f;
        auto &top_left_panel_container_layout = top_left_panel_container->getComponentMutable<core::components::Layout>();
        top_left_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        top_left_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        top_left_panel_container_layout.height.size = 0.1f;
        top_left_panel_container_layout.direction = core::components::Layout::Direction::Horizontal;
        // top_left_panel_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        // top_left_panel_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        top_left_panel_container_layout.padding = { 16, 16, 8, 8 };
        topbar_container_layout.child_gap = 12;
        top_left_panel_container->rename("top left panel container");
        top_left_panel_container->setParent(*left_panel);

        auto left_panel_search_bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &left_panel_search_bar_rect = left_panel_search_bar->getComponentMutable<core::components::UIRect>();
        left_panel_search_bar_rect.color = core::types::Color::BLACK;
        left_panel_search_bar_rect.color.a = 0.30f;
        auto &left_panel_search_bar_layout = left_panel_search_bar->getComponentMutable<core::components::Layout>();
        left_panel_search_bar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        left_panel_search_bar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        left_panel_search_bar_layout.height.size = 0.4f;
        left_panel_search_bar->rename("search bar");
        left_panel_search_bar->setParent(*top_left_panel_container);

        auto left_panel_pin = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &left_panel_pin_rect = left_panel_pin->getComponentMutable<core::components::UIRect>();
        left_panel_pin_rect.color = core::types::Color::BLACK;
        left_panel_pin_rect.color.a = 1.0f;
        auto &left_panel_pin_layout = left_panel_pin->getComponentMutable<core::components::Layout>();
        left_panel_pin_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        left_panel_pin_layout.width.size = core::components::Layout::SizingAxis::MinMax(12.0f, 12.0f);
        left_panel_pin_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        left_panel_pin_layout.height.size = core::components::Layout::SizingAxis::MinMax(12.0f, 12.0f);
        left_panel_pin->rename("pin");
        left_panel_pin->setParent(*top_left_panel_container);

        auto content_left_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &content_left_panel_container_rect = content_left_panel_container->getComponentMutable<core::components::UIRect>();
        content_left_panel_container_rect.color = core::types::Color::WHITE;
        content_left_panel_container_rect.color.a = 1.0f;
        auto &content_left_panel_container_layout = content_left_panel_container->getComponentMutable<core::components::Layout>();
        content_left_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        content_left_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        content_left_panel_container_layout.direction = core::components::Layout::Direction::Vertical;
        content_left_panel_container_layout.child_gap = 8;
        content_left_panel_container_layout.padding = { 16, 16, 0, 0 };
        content_left_panel_container->rename("content left panel container");
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
        add_node_button->rename("add node button");
        add_node_button->setParent(*content_left_panel_container);

        auto scene_viewport_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &scene_viewport_container_rect = scene_viewport_container->getComponentMutable<core::components::UIRect>();
        scene_viewport_container_rect.color.a = 0.0f;
        auto &scene_viewport_container_layout = scene_viewport_container->getComponentMutable<core ::components::Layout>();
        scene_viewport_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_viewport_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        scene_viewport_container_layout.direction = core::components::Layout::Direction::Vertical;
        scene_viewport_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Start;
        scene_viewport_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Start;
        scene_viewport_container_layout.child_gap = 0;
        scene_viewport_container_layout.padding = { 0, 0, 8, 16 };
        scene_viewport_container->rename("scene viewport container");
        scene_viewport_container->setParent(*content_left_panel_container);

        for (auto &entity : scene->getChildren()) {
            fodableTreeinit(entity, *scene_viewport_container);
        }

        // auto button1 = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        // spdlog::info("child count: {}", button1->getChildren().size());
        // auto children = button1->getChildren();
        // if (!children.empty() && children[0].isAlive()) {
        //     spdlog::info("Destroying child");
        //     children[0].destroy();
        //     spdlog::info("caca");
        // }
        // button1->getSignal<core::ecs::entities::UIButton &>("ToIdle").connect([](core::ecs::entities::UIButton &btn) {
        //     auto &rect = btn.getComponentMutable<core::components::UIRect>();
        //     rect.color = core::types::Color::GREEN;
        //     rect.color.a = 0.2f;

        //     rect.border.color = core::types::Color::BLACK;
        // });
        // button1->getSignal<core::ecs::entities::UIButton &>("ToIdle").emit(*button1);
        // auto &button_layout1 = button1->getComponentMutable<core::components::Layout>();
        // button_layout1.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        // button_layout1.width.size = core::components::Layout::SizingAxis::MinMax{ 100.0f, 100.0f };
        // button_layout1.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        // button_layout1.height.size = core::components::Layout::SizingAxis::MinMax{ 50.0f, 50.0f };
        // button1->rename("buttton1");
        // button1->setParent(*floating_window);

        // button1->getSignal<core::ecs::entities::UIButton &>("Pressed").connect([floating_window](core::ecs::entities::UIButton &btn) {
        //     auto &window_state = floating_window->getComponentMutable<core::components::UIPanelState>();
        //     if (window_state.open) {
        //         floating_window->getSignal<core::ecs::entities::UIPanel &>("Close").emit(*floating_window);
        //     } else {
        //         floating_window->getSignal<core::ecs::entities::UIPanel &>("Open").emit(*floating_window);
        //     }
        // });

        // auto middle_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        // auto &middle_panel_container_rect = middle_panel_container->getComponentMutable<core::components::UIRect>();
        // middle_panel_container_rect.color = core::types::Color::WHITE;
        // middle_panel_container_rect.color.a = 0.0f;
        // auto &middle_panel_container_layout = middle_panel_container->getComponentMutable<core::components::Layout>();
        // middle_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        // middle_panel_container_layout.width.size = 0.6f;
        // middle_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        // middle_panel_container_layout.height.size = 0.1f;
        // middle_panel_container_layout.direction = core::components::Layout::Direction::Horizontal;
        // middle_panel_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        // middle_panel_container_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        // left_panel_container_layout.padding = { 0, 0, 0, 16 };
        // middle_panel_container->rename("middle panel container");
        // middle_panel_container->setParent(*scene_ui_container);

        // auto middle_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPanel>("Entity::UI::UIRect::UIPanel");
        // auto &middle_panel_rect = middle_panel->getComponentMutable<core::components::UIRect>();
        // middle_panel_rect.color = core::types::Color::WHITE;
        // middle_panel_container_rect.corner_radius = { 4, 4, 4, 4 };
        // auto &middle_panel_layout = middle_panel->getComponentMutable<core::components::Layout>();
        // middle_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        // middle_panel_layout.width.size = 0.3f;
        // middle_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        // middle_panel_layout.height.size = 0.6f;
        // middle_panel_container_layout.direction = core::components::Layout::Direction::Horizontal;
        // middle_panel->rename("middle panel");
        // middle_panel->setParent(*middle_panel_container);

        // auto right_panel_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        // auto &right_panel_container_rect = right_panel_container->getComponentMutable<core::components::UIRect>();
        // right_panel_container_rect.color = core::types::Color::WHITE;
        // right_panel_container_rect.color.a = 0.0f;
        // auto &right_panel_container_layout = right_panel_container->getComponentMutable<core::components::Layout>();
        // right_panel_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        // right_panel_container_layout.width.size = 0.2f;
        // right_panel_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        // right_panel_container_layout.height.size = 1.0f;
        // right_panel_container_layout.direction = core::components::Layout::Direction::Horizontal;
        // right_panel_container_layout.padding = { 0, 16, 8, 16 };
        // right_panel_container->rename("right panel container");
        // right_panel_container->setParent(*scene_ui_container);

        // auto right_panel = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPanel>("Entity::UI::UIRect::UIPanel");
        // auto &right_panel_rect = right_panel->getComponentMutable<core::components::UIRect>();
        // right_panel_rect.color = core::types::Color::WHITE;
        // right_panel_container_rect.corner_radius = { 4, 4, 4, 4 };
        // auto &right_panel_layout = right_panel->getComponentMutable<core::components::Layout>();
        // right_panel_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        // right_panel_layout.width.size = 1.0f;
        // right_panel_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        // right_panel_layout.height.size = 1.0f;
        // right_panel_container_layout.direction = core::components::Layout::Direction::Vertical;
        // right_panel->rename("right panel");
        // right_panel->setParent(*right_panel_container);

        // auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        // label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        // label->setText("Hello, World!");
        // label->setFontSize(48);
        // label->rename("hello world");
        // label->setParent(*floating_window);
        // auto &label_layout = label->getComponentMutable<core::components::Layout>();

        // spdlog::info(glz::write<glz::opts{ .prettify = true }>(scene->serialize()).value());
    }

    void EditorManager::openNewEditorSelectionPopup()
    {
        auto open_editor_popup = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPopup>("Entity::UI::UIRect::UIPopup");
        open_editor_popup->setParent(*m_engine.getECS().getCurrentScene());
        auto open_editor_bg = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        open_editor_bg->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        open_editor_bg->getComponentMutable<core::components::Layout>().width.size = 0.5f;
        open_editor_bg->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        open_editor_bg->getComponentMutable<core::components::Layout>().height.size = 0.5f;
        open_editor_bg->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Vertical;
        open_editor_bg->getComponentMutable<core::components::Layout>().padding = { 8, 8, 8, 8 };
        open_editor_bg->setParent(*open_editor_popup);
        auto open_editor_top_bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        open_editor_top_bar->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Horizontal;
        open_editor_top_bar->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        open_editor_top_bar->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        open_editor_top_bar->setParent(*open_editor_bg);

        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        label->setText("atmo.open_new_editor");
        label->setFontBold(false);
        label->setFontSize(16);
        label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        label->setParent(*open_editor_top_bar);
        auto close_btn_holder = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        close_btn_holder->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_btn_holder->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        close_btn_holder->getComponentMutable<core::components::Layout>().child_alignment.horizontal = core::components::Layout::ChildAlignment::End;
        close_btn_holder->setParent(*open_editor_top_bar);
        auto close_open_editor_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &close_open_editor_btn_rect = close_open_editor_btn->getComponentMutable<core::components::UIRect>();
        close_open_editor_btn_rect.color = core::types::Color::BLACK;
        auto &close_open_editor_btn_layout = close_open_editor_btn->getComponentMutable<core::components::Layout>();
        close_open_editor_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        close_open_editor_btn_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 26.0f, 26.0f };
        close_open_editor_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        close_open_editor_btn_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 26.0f, 26.0f };
        close_open_editor_btn->getChildren()[0].destroy();
        close_open_editor_btn->setParent(*close_btn_holder);
        close_open_editor_btn->getSignal<>("Released").connect([open_editor_popup]() { open_editor_popup->destroy(); });
    }
} // namespace atmo::editor

#endif
