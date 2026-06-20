#include "editor_manager.hpp"
#include <cstdint>
#include <vector>
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
#include "editor/editor_registry.hpp"
#include "flecs/addons/cpp/entity.hpp"
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

        auto topbar_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &topbar_container_layout = topbar_container->getComponentMutable<core::components::Layout>();
        topbar_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        topbar_container_layout.width.size = 1.0f;
        topbar_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        topbar_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 50.0f, 50.0f };
        topbar_container_layout.direction = core::components::Layout::Direction::Horizontal;
        topbar_container_layout.padding = { 8, 8, 8, 8 };
        topbar_container->rename("topbar container");
        topbar_container->setParent(*window_ui_container);

        m_topbar = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &topbar_rect = m_topbar->getComponentMutable<core::components::UIRect>();
        topbar_rect.color = core::types::Color::WHITE;
        auto &topbar_layout = m_topbar->getComponentMutable<core::components::Layout>();
        topbar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        topbar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        topbar_layout.padding = { 4, 4, 4, 4 };
        topbar_layout.child_gap = 4;
        m_topbar->setParent(*topbar_container);

        updateTopBar();

        m_editor_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &editor_container_layout = m_editor_container->getComponentMutable<core::components::Layout>();
        editor_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        editor_container_layout.width.size = 1.0f;
        editor_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        m_editor_container->rename("scene ui container");
        m_editor_container->setParent(*window_ui_container);
        // spdlog::info(glz::write<glz::opts{ .prettify = true }>(scene->serialize()).value());
    }

    void EditorManager::updateTopBar()
    {
        for (auto &child : m_topbar->getChildren()) child.destroy();

        std::uint64_t index = 0;
        for (auto editor : m_editors) {
            auto editor_select = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto &editor_select_rect = editor_select->getComponentMutable<core::components::UIRect>();
            editor_select_rect.color = core::types::Color("#868686");
            auto &editor_select_layout = editor_select->getComponentMutable<core::components::Layout>();
            editor_select_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            editor_select_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            ((core::ecs::entities::UILabel)editor_select->getChildren()[0]).setText(std::string(editor->name()));
            auto &editor_select_btn = editor_select->getComponentMutable<core::components::UIButton>();
            editor_select_btn.group = 2;
            editor_select_btn.toggle = true;
            editor_select->setParent(*m_topbar);
            editor_select->getSignal<>("Released").connect([]() { spdlog::info("caca"); });
            editor_select->getSignal<int>("Toggle").connect([this, editor_select, index, editor_name = std::string(editor->name())](int group) {
                auto &btn_cmp = editor_select->getComponentMutable<core::components::UIButton>();
                auto &btn_rect = editor_select->getComponentMutable<core::components::UIRect>();
                if (btn_cmp.is_pressed) {
                    btn_rect.color = core::types::Color("#b25959");
                    m_editor_containers[index]->getComponentMutable<core::components::UI>().visible = true;
                } else {
                    btn_rect.color = core::types::Color("#868686");
                    m_editor_containers[index]->getComponentMutable<core::components::UI>().visible = false;
                }
                spdlog::info("{}: {}", editor_name, btn_cmp.is_pressed);
            });
            index++;
        }

        auto open_editor_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &open_editor_btn_rect = open_editor_btn->getComponentMutable<core::components::UIRect>();
        open_editor_btn_rect.color = core::types::Color::BLACK;
        auto &open_editor_btn_layout = open_editor_btn->getComponentMutable<core::components::Layout>();
        open_editor_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        open_editor_btn_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 26.0f, 26.0f };
        open_editor_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        open_editor_btn_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 26.0f, 26.0f };
        open_editor_btn->getChildren()[0].destroy();
        open_editor_btn->setParent(*m_topbar);
        open_editor_btn->getSignal<>("Released").connect([this]() { openNewEditorSelectionPopup(); });
    }

    void EditorManager::openNewEditorSelectionPopup()
    {
        auto open_editor_popup = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPopup>("Entity::UI::UIRect::UIPopup");
        open_editor_popup->setParent(*m_engine.getECS().getCurrentScene());
        auto open_editor_bg = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        open_editor_bg->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        open_editor_bg->getComponentMutable<core::components::Layout>().width.size = 0.35f;
        open_editor_bg->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        open_editor_bg->getComponentMutable<core::components::Layout>().height.size = 0.75f;
        open_editor_bg->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Vertical;
        open_editor_bg->getComponentMutable<core::components::Layout>().padding = { 8, 8, 8, 8 };
        open_editor_bg->getComponentMutable<core::components::Layout>().child_gap = 8;
        open_editor_bg->getComponentMutable<core::components::UIRect>().color = core::types::Color("#9f9f9f");
        open_editor_bg->setParent(*open_editor_popup);
        auto open_editor_top_bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        open_editor_top_bar->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Horizontal;
        open_editor_top_bar->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        open_editor_top_bar->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        open_editor_top_bar->getComponentMutable<core::components::Layout>().height.size = core::components::Layout::SizingAxis::MinMax{ 32.0f, 32.0f };
        open_editor_top_bar->setParent(*open_editor_bg);

        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        label->setText("atmo.open_new_editor");
        label->setFontBold(false);
        label->setFontSize(16);
        label->setParent(*open_editor_top_bar);
        auto close_btn_holder = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        close_btn_holder->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_btn_holder->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_btn_holder->getComponentMutable<core::components::Layout>().child_alignment.horizontal = core::components::Layout::ChildAlignment::End;
        close_btn_holder->setParent(*open_editor_top_bar);
        auto close_open_editor_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &close_open_editor_btn_rect = close_open_editor_btn->getComponentMutable<core::components::UIRect>();
        close_open_editor_btn_rect.color = core::types::Color::RED;
        auto &close_open_editor_btn_layout = close_open_editor_btn->getComponentMutable<core::components::Layout>();
        close_open_editor_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_open_editor_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_open_editor_btn_layout.aspect_ratio = { 1.0f, 1.0f };
        close_open_editor_btn->getChildren()[0].destroy();
        close_open_editor_btn->setParent(*close_btn_holder);
        close_open_editor_btn->getSignal<>("Released").connect([open_editor_popup]() { open_editor_popup->destroy(); });

        auto editor_creation_button_list = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        editor_creation_button_list->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Vertical;
        editor_creation_button_list->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        editor_creation_button_list->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        editor_creation_button_list->getComponentMutable<core::components::Layout>().child_gap = 8;
        editor_creation_button_list->setParent(*open_editor_bg);

        for (auto t : EditorRegistry::GetEntries()) {
            auto create_btn = makeEditorCreationButton(t);
            create_btn.getSignal<>("Released").connect([open_editor_popup]() { open_editor_popup->destroy(); });
            create_btn.setParent(*editor_creation_button_list);
        }
    }

    core::ecs::entities::UIButton EditorManager::makeEditorCreationButton(const std::string &editor)
    {
        auto new_editor = EditorRegistry::Create(editor);

        auto open_editor_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &open_editor_btn_rect = open_editor_btn->getComponentMutable<core::components::UIRect>();
        auto &open_editor_btn_layout = open_editor_btn->getComponentMutable<core::components::Layout>();
        open_editor_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        open_editor_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        open_editor_btn_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 120.0f, 0.0f };
        open_editor_btn_layout.padding = { 8, 8, 8, 8 };
        open_editor_btn_layout.child_gap = 8;
        open_editor_btn_layout.direction = core::components::Layout::Direction::Vertical;
        open_editor_btn->getChildren()[0].destroy();
        open_editor_btn->getSignal<>("Released").connect([this, new_editor]() {
            std::uint64_t index = m_editors.size();
            m_editors.emplace_back(new_editor);

            auto editor_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            auto &editor_container_layout = editor_container->getComponentMutable<core::components::Layout>();
            editor_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            editor_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            editor_container->setParent(*m_editor_container);
            m_editor_containers.emplace_back(editor_container);

            new_editor->init(*editor_container);
            updateTopBar();
        });

        auto open_editor_topbar = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        open_editor_topbar->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        open_editor_topbar->getComponentMutable<core::components::Layout>().child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        open_editor_topbar->getComponentMutable<core::components::Layout>().child_gap = 8;
        open_editor_topbar->setParent(*open_editor_btn);

        auto open_editor_image = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        open_editor_image->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        open_editor_image->getComponentMutable<core::components::Layout>().width.size = core::components::Layout::SizingAxis::MinMax{ 32.0f, 32.0f };
        open_editor_image->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        open_editor_image->getComponentMutable<core::components::Layout>().aspect_ratio = { 1.0f, 1.0f };
        open_editor_image->getComponentMutable<core::components::UIRect>().color = core::types::Color::GREEN;
        open_editor_image->setParent(*open_editor_topbar);

        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        label->setText(std::string(new_editor->name()));
        label->setFontBold(false);
        label->setFontSize(12);
        label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        label->setParent(*open_editor_topbar);

        auto description = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        description->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        description->setText(std::string(new_editor->description()));
        description->setFontBold(false);
        description->setFontSize(9);
        description->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        description->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        description->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        description->setParent(*open_editor_btn);

        return *open_editor_btn;
    }
} // namespace atmo::editor

#endif
