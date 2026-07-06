#include "editor_manager.hpp"
#include <cstdint>
#include <vector>
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_checkbox/ui_checkbox.hpp"
#include "core/ecs/entities/ui/ui_foldable_tree_item/ui_foldable_tree_item.hpp"
#include "core/ecs/entities/ui/ui_image/ui_image.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/progress_tick_event/progress_tick_event.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_panel/ui_panel.hpp"
#include "editor/editor_entities/ui_popup/ui_popup.hpp"
#include "editor/editor_registry.hpp"
#include "editor/editors/scene_editor/scene_editor.hpp"
#include "editor/inspector_utils.hpp"
#include "flecs/addons/cpp/entity.hpp"
#include "glaze/json/prettify.hpp"
#include "locale/locale_manager.hpp"
#include "meta/meta_registry.hpp"
#include "project/file_system.hpp"
#include "project/project_manager.hpp"
#include "project/project_settings.hpp"
#include "spdlog/spdlog.h"

#if !defined(ATMO_EXPORT)

#include "SDL3/SDL_dialog.h"
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
                .action = [this] { handleSave(); },
            });

        m_commands.registerCommand(
            {
                .id = "atmo.commands.file.save_as",
                .category = "atmo.commands.file.category",
                .shortcut = Shortcut{ SDLK_S, static_cast<SDL_Keymod>(PRIMARY_MOD | SDL_KMOD_SHIFT) },
                .action = [this] { handleSaveAs(); },
            });

        m_commands.registerCommand(
            {
                .id = "atmo.commands.file.open",
                .category = "atmo.commands.file.category",
                .shortcut = Shortcut{ SDLK_O, PRIMARY_MOD },
                .action = [this] { handleOpen(); },
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

        m_commands.registerCommand(
            {
                .id = "atmo.commands.edit.project_settings",
                .category = "atmo.commands.edit.category",
                .shortcut = Shortcut{ SDLK_P, static_cast<SDL_Keymod>(PRIMARY_MOD | SDL_KMOD_SHIFT) },
                .action = [this] { core::SignalQueue::Enqueue([this]() { openProjectSettings(); }); },
            });

        if (spdlog::default_logger()->level() == spdlog::level::debug) {
            m_commands.registerCommand(
                {
                    .id = "atmo.commands.debug.create_demo_entities",
                    .category = "atmo.commands.debug.category",
                    .shortcut = Shortcut{ SDLK_D, static_cast<SDL_Keymod>(PRIMARY_MOD | SDL_KMOD_SHIFT) },
                    .action =
                        [this] {
                            core::SignalQueue::Enqueue([this]() {
                                if (m_active_editor_index && m_editors[m_active_editor_index.value()]->getTypeName() == "Editor::SceneEditor") {
                                    std::static_pointer_cast<atmo::editor::SceneEditor>(m_editors[m_active_editor_index.value()])->createDemoEntities();
                                }
                            });
                        },
                });
        }
    }

    void EditorManager::init()
    {
        registerDefaultCommands();

        auto scene = m_engine.getECS().getCurrentScene();

        m_menu_bar = makePlatformMenuBar();
        m_menu_bar->build(*scene, m_commands);

        auto window_ui_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &window_ui_container_layout = window_ui_container->getComponentMutable<core::components::Layout>();
        window_ui_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        window_ui_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        window_ui_container_layout.direction = core::components::Layout::Direction::Vertical;
        window_ui_container_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::Center;
        window_ui_container->setParent(*scene);

        auto topbar_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &topbar_container_layout = topbar_container->getComponentMutable<core::components::Layout>();
        topbar_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        topbar_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        topbar_container_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 60.0f, 60.0f };
        topbar_container_layout.direction = core::components::Layout::Direction::Horizontal;
        topbar_container_layout.padding = { 16, 16, 16, 16 };
        topbar_container->setParent(*window_ui_container);

        m_topbar = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &topbar_rect = m_topbar->getComponentMutable<core::components::UIRect>();
        topbar_rect.color = core::types::Color::WHITE;
        topbar_rect.corner_radius = { 4, 4, 4, 4 };
        auto &topbar_layout = m_topbar->getComponentMutable<core::components::Layout>();
        topbar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        topbar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        topbar_layout.padding = { 8, 8, 8, 8 };
        topbar_layout.child_gap = 8;
        m_topbar->setParent(*topbar_container);

        updateTopBar();

        m_editor_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &editor_container_layout = m_editor_container->getComponentMutable<core::components::Layout>();
        editor_container_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        editor_container_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        m_editor_container->rename("scene ui container");
        m_editor_container->setParent(*window_ui_container);

        makeToolbar(*window_ui_container);

        core::SignalQueue::Enqueue([scene]() { scene->getParent<core::ecs::entities::Window>().primeScrollContainers(); });

        core::event::EventRegistry::SetCallBack<core::event::events::ProgressTickEvent>([this](core::event::events::ProgressTickEvent *) {
            if (m_play_process && !m_play_process->isRunning()) {
                m_play_process.reset();
                m_play_btn_icon->setTexturePath("project://assets/icons/play.svg");
            }
        });
    }

    void EditorManager::updateTopBar()
    {
        for (auto &child : m_topbar->getChildren()) child.destroy();

        std::uint64_t index = 0;
        for (auto editor : m_editors) {
            auto editor_select = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
            auto &editor_select_rect = editor_select->getComponentMutable<core::components::UIRect>();
            auto &editor_select_layout = editor_select->getComponentMutable<core::components::Layout>();
            editor_select_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
            editor_select_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            editor_select_layout.child_gap = 8;
            ((core::ecs::entities::UILabel)editor_select->getChildren()[0]).setText(std::string(editor->name()));
            auto &editor_select_btn = editor_select->getComponentMutable<core::components::UIButton>();
            editor_select_btn.group = 2;
            editor_select_btn.toggle = true;
            editor_select->setParent(*m_topbar);
            editor_select->getSignal<bool>("Toggle").connect([this, editor_select, index, editor_name = std::string(editor->name())](bool new_state) {
                auto &btn_rect = editor_select->getComponentMutable<core::components::UIRect>();
                if (new_state) {
                    btn_rect.color = core::types::Color("#b25959");
                    m_editor_containers[index]->getComponentMutable<core::components::UI>().visible = true;
                    m_active_editor_index = index;
                } else {
                    btn_rect.color = core::types::Color("#868686");
                    m_editor_containers[index]->getComponentMutable<core::components::UI>().visible = false;
                }
            });
            auto editor_icon = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
            editor_icon->getComponentMutable<core::components::UIImage>().texture_path = editor->iconPath();
            editor_icon->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            auto &editor_icon_layout = editor_icon->getComponentMutable<core::components::Layout>();
            editor_icon_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
            editor_icon_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 20.0f, 20.0f };
            editor_icon_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            editor_icon_layout.aspect_ratio = { 1.0f, 1.0f };
            editor_icon->setParent(*editor_select);
            editor_icon->swap(editor_select->getChildren()[0]);
            index++;
        }

        auto open_editor_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &open_editor_btn_rect = open_editor_btn->getComponentMutable<core::components::UIRect>();
        open_editor_btn_rect.color.a = 0.0f;
        auto &open_editor_btn_layout = open_editor_btn->getComponentMutable<core::components::Layout>();
        open_editor_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        open_editor_btn_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 26.0f, 26.0f };
        open_editor_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        open_editor_btn_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 26.0f, 26.0f };
        open_editor_btn->getChildren()[0].destroy();
        open_editor_btn->setParent(*m_topbar);
        open_editor_btn->getSignal<>("Released").connect([this]() { openNewEditorSelectionPopup(); });

        auto open_editor_btn_image = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
        open_editor_btn_image->getComponentMutable<core::components::UIImage>().texture_path = "project://assets/icons/plus.svg";
        open_editor_btn_image->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        open_editor_btn_image->setParent(*open_editor_btn);
    }

    void EditorManager::makeToolbar(core::ecs::entities::UI container)
    {
        auto toolbar_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        toolbar_container->getComponentMutable<core::components::Layout>().padding = { 16, 16, 16, 16 };
        toolbar_container->setParent(container);

        m_toolbar = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        m_toolbar->getComponentMutable<core::components::UIRect>().color = core::types::Color::WHITE;
        m_toolbar->getComponentMutable<core::components::UIRect>().corner_radius = { 4.0f, 4.0f, 4.0f, 4.0f };
        m_toolbar->getComponentMutable<core::components::Layout>().child_gap = 4;
        m_toolbar->getComponentMutable<core::components::Layout>().padding = { 4, 4, 4, 4 };
        m_toolbar->getComponentMutable<core::components::Layout>().child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        m_toolbar->setParent(*toolbar_container);

        auto editor_tools_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        editor_tools_container->getComponentMutable<core::components::Layout>().child_gap = 4;
        editor_tools_container->setParent(*m_toolbar);

        auto editor_tools_spacer = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
        editor_tools_spacer->getComponentMutable<core::components::UIImage>().texture_path = "project://assets/icons/dot.svg";
        editor_tools_spacer->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        editor_tools_spacer->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        editor_tools_spacer->getComponentMutable<core::components::Layout>().width.size = core::components::Layout::SizingAxis::MinMax{ 24.0f, 24.0f };
        editor_tools_spacer->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        editor_tools_spacer->getComponentMutable<core::components::Layout>().height.size = core::components::Layout::SizingAxis::MinMax{ 24.0f, 24.0f };
        editor_tools_spacer->setParent(*m_toolbar);

        auto engine_tools_container = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        engine_tools_container->getComponentMutable<core::components::Layout>().child_gap = 4;
        engine_tools_container->setParent(*m_toolbar);

        m_play_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        m_play_btn->getChildren()[0].destroy();
        m_play_btn->setParent(*engine_tools_container);
        m_play_btn->getSignal<>("Released").connect([this]() {
            if (m_play_process && m_play_process->isRunning())
                stopPlay();
            else
                startPlay();
        });

        m_play_btn_icon = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
        m_play_btn_icon->setTexturePath("project://assets/icons/play.svg");
        m_play_btn_icon->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        m_play_btn_icon->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        m_play_btn_icon->getComponentMutable<core::components::Layout>().width.size = core::components::Layout::SizingAxis::MinMax{ 24.0f, 24.0f };
        m_play_btn_icon->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        m_play_btn_icon->getComponentMutable<core::components::Layout>().height.size = core::components::Layout::SizingAxis::MinMax{ 24.0f, 24.0f };
        m_play_btn_icon->setParent(*m_play_btn);
    }

    void EditorManager::startPlay()
    {
        if (m_play_process && m_play_process->isRunning())
            return;

        m_play_process.emplace();
        if (!m_play_process->spawn(project::FileSystem::GetRootPath(), { "--project", m_project_path, "--run" })) {
            spdlog::error("Failed to launch play-mode process");
            m_play_process.reset();
            return;
        }

        m_play_btn_icon->setTexturePath("project://assets/icons/square.svg");
    }

    void EditorManager::stopPlay()
    {
        if (m_play_process) {
            m_play_process->terminate();
            m_play_process.reset();
        }

        m_play_btn_icon->setTexturePath("project://assets/icons/play.svg");
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
        label->setFontSize(24);
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

            ((core::ecs::entities::UIButton)m_topbar->getChildren()[index]).press();
        });

        auto open_editor_topbar = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        open_editor_topbar->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        open_editor_topbar->getComponentMutable<core::components::Layout>().child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
        open_editor_topbar->getComponentMutable<core::components::Layout>().child_gap = 8;
        open_editor_topbar->setParent(*open_editor_btn);

        auto open_editor_image = core::ecs::EntityRegistry::Create<core::ecs::entities::UIImage>("Entity::UI::UIImage");
        open_editor_image->getComponentMutable<core::components::UIImage>().texture_path = new_editor->iconPath();
        open_editor_image->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        open_editor_image->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        open_editor_image->getComponentMutable<core::components::Layout>().width.size = core::components::Layout::SizingAxis::MinMax{ 32.0f, 32.0f };
        open_editor_image->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        open_editor_image->getComponentMutable<core::components::Layout>().aspect_ratio = { 1.0f, 1.0f };
        open_editor_image->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        open_editor_image->setParent(*open_editor_topbar);

        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        label->setText(std::string(new_editor->name()));
        label->setFontSize(18);
        label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        label->setParent(*open_editor_topbar);

        auto description = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        description->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        description->setText(std::string(new_editor->description()));
        description->setFontSize(13);
        description->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        description->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        description->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        description->setParent(*open_editor_btn);

        return *open_editor_btn;
    }

    std::shared_ptr<Editor> EditorManager::activeEditor() const
    {
        if (m_active_editor_index && *m_active_editor_index < m_editors.size())
            return m_editors[*m_active_editor_index];
        return nullptr;
    }

    static void HandleFileDialogResult(void *userdata, const char *const *filelist, int /*filter*/)
    {
        std::unique_ptr<std::function<void(const std::string &)>> callback(static_cast<std::function<void(const std::string &)> *>(userdata));

        if (!filelist) {
            spdlog::error("File dialog error: {}", SDL_GetError());
            return;
        }
        if (!filelist[0])
            return;

        try {
            (*callback)(filelist[0]);
        } catch (const std::exception &e) {
            spdlog::error("File dialog action failed: {}", e.what());
        }
    }

    void EditorManager::handleSave()
    {
        auto editor = activeEditor();
        if (!editor) {
            spdlog::warn("No active editor to save.");
            return;
        }

        if (editor->hasFilePath()) {
            editor->save();
        } else {
            handleSaveAs();
        }
    }

    void EditorManager::handleSaveAs()
    {
        auto editor = activeEditor();
        if (!editor)
            return;

        auto *callback = new std::function<void(const std::string &)>([editor](const std::string &path) { editor->saveAs(path); });
        SDL_ShowSaveFileDialog(&HandleFileDialogResult, callback, nullptr, nullptr, 0, m_project_path.c_str());
    }

    void EditorManager::handleOpen()
    {
        auto editor = activeEditor();
        if (!editor)
            return;

        auto *callback = new std::function<void(const std::string &)>([editor](const std::string &path) { editor->open(path); });
        SDL_ShowOpenFileDialog(&HandleFileDialogResult, callback, nullptr, nullptr, 0, m_project_path.c_str(), false);
    }

    std::shared_ptr<core::ecs::entities::UIFoldableTreeItem> EditorManager::makeSettingsSection(core::ecs::entities::UI &body, const std::string &title)
    {
        auto section = core::ecs::EntityRegistry::Create<core::ecs::entities::UIFoldableTreeItem>("Entity::UI::UIRect::UIFoldableTreeItem");
        section->getTitleLabel().setText(title);
        auto &section_layout = section->getComponentMutable<core::components::Layout>();
        section_layout.direction = core::components::Layout::Direction::Vertical;
        section_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        section_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        section->setParent(body);
        return section;
    }

    void EditorManager::openProjectSettings()
    {
        m_settings_draft = std::make_shared<project::ProjectSettings>(project::ProjectManager::GetSettings());
        auto draft = m_settings_draft;

        auto project_settings_popup = core::ecs::EntityRegistry::Create<core::ecs::entities::UIPopup>("Entity::UI::UIRect::UIPopup");
        project_settings_popup->setParent(*m_engine.getECS().getCurrentScene());

        auto project_settings = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &project_settings_rect = project_settings->getComponentMutable<core::components::UIRect>();
        project_settings_rect.corner_radius = { 4, 4, 4, 4 };
        auto &project_settings_layout = project_settings->getComponentMutable<core::components::Layout>();
        project_settings_layout.direction = core::components::Layout::Direction::Vertical;
        project_settings_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        project_settings_layout.width.size = 0.5f;
        project_settings_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::PERCENT;
        project_settings_layout.height.size = 0.7f;
        project_settings_layout.padding = { 4, 4, 4, 4 };
        project_settings->setParent(*project_settings_popup);

        auto project_settings_top_bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        project_settings_top_bar->getComponentMutable<core::components::Layout>().direction = core::components::Layout::Direction::Horizontal;
        project_settings_top_bar->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        project_settings_top_bar->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        project_settings_top_bar->getComponentMutable<core::components::Layout>().height.size = core::components::Layout::SizingAxis::MinMax{ 32.0f, 32.0f };
        project_settings_top_bar->setParent(*project_settings);

        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setText("atmo.project_settings");
        label->setFontSize(24);
        label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
        label->setParent(*project_settings_top_bar);
        auto close_btn_holder = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        close_btn_holder->getComponentMutable<core::components::Layout>().width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_btn_holder->getComponentMutable<core::components::Layout>().height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_btn_holder->getComponentMutable<core::components::Layout>().child_alignment.horizontal = core::components::Layout::ChildAlignment::End;
        close_btn_holder->setParent(*project_settings_top_bar);
        auto close_project_settings_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &close_project_settings_btn_rect = close_project_settings_btn->getComponentMutable<core::components::UIRect>();
        close_project_settings_btn_rect.color = core::types::Color::RED;
        close_project_settings_btn_rect.corner_radius = { 4, 4, 4, 4 };
        auto &close_project_settings_btn_layout = close_project_settings_btn->getComponentMutable<core::components::Layout>();
        close_project_settings_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_project_settings_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        close_project_settings_btn_layout.aspect_ratio = { 1.0f, 1.0f };
        close_project_settings_btn->getChildren()[0].destroy();
        close_project_settings_btn->setParent(*close_btn_holder);
        close_project_settings_btn->getSignal<>("Released").connect([project_settings_popup]() {
            core::SignalQueue::Enqueue([project_settings_popup]() { project_settings_popup->destroy(); });
        });

        auto body = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &body_layout = body->getComponentMutable<core::components::Layout>();
        body_layout.direction = core::components::Layout::Direction::Vertical;
        body_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        body_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        body_layout.clip.vertical = true;
        body_layout.child_gap = 8;
        body_layout.padding = { 0, 0, 8, 8 };
        body->setParent(*project_settings);

        buildFieldWidgetRows(meta::MetaRegistry::Instance().lookup<project::App>(), &draft->app, makeSettingsSection(*body, "App")->getChildContainer());
        buildFieldWidgetRows(meta::MetaRegistry::Instance().lookup<project::Boot>(), &draft->boot, makeSettingsSection(*body, "Boot")->getChildContainer());
        buildFieldWidgetRows(
            meta::MetaRegistry::Instance().lookup<project::Window>(), &draft->window, makeSettingsSection(*body, "Window")->getChildContainer());
        buildFieldWidgetRows(
            meta::MetaRegistry::Instance().lookup<project::Singletons>(), &draft->singletons, makeSettingsSection(*body, "Singletons")->getChildContainer());
        buildFieldWidgetRows(
            meta::MetaRegistry::Instance().lookup<project::Engine>(), &draft->engine, makeSettingsSection(*body, "Engine")->getChildContainer());
        buildFieldWidgetRows(meta::MetaRegistry::Instance().lookup<project::Debug>(), &draft->debug, makeSettingsSection(*body, "Debug")->getChildContainer());

        auto addons_container = makeSettingsSection(*body, "Addons")->getChildContainer();
        for (auto &[addon_name, enabled] : draft->addons.addons) {
            auto row = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
            auto &row_layout = row->getComponentMutable<core::components::Layout>();
            row_layout.direction = core::components::Layout::Direction::Horizontal;
            row_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
            row_layout.child_gap = 8;
            row_layout.child_alignment.vertical = core::components::Layout::ChildAlignment::Center;
            row->setParent(addons_container);

            auto addon_label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
            addon_label->setText(addon_name);
            addon_label->setFontSize(12);
            addon_label->getComponentMutable<core::components::UI>().modulate = core::types::Color::BLACK;
            addon_label->setParent(*row);

            auto addon_checkbox = core::ecs::EntityRegistry::Create<core::ecs::entities::UICheckBox>("Entity::UI::UIRect::UICheckBox");
            auto &checkbox_comp = addon_checkbox->getComponentMutable<core::components::UICheckBox>();
            checkbox_comp.trigger = enabled;
            addon_checkbox->getComponentMutable<core::components::UIRect>().color = enabled ? core::types::Color::WHITE : core::types::Color::BLACK;
            auto &checkbox_layout = addon_checkbox->getComponentMutable<core::components::Layout>();
            checkbox_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
            checkbox_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 20.0f, 20.0f };
            checkbox_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
            checkbox_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 20.0f, 20.0f };
            addon_checkbox->setParent(*row);

            std::string addon_key = addon_name;
            addon_checkbox->getSignal<core::ecs::entities::UICheckBox &>("Clicked").connect([draft, addon_key](core::ecs::entities::UICheckBox &chbox) {
                draft->addons.addons[addon_key] = chbox.getComponentMutable<core::components::UICheckBox>().trigger;
            });
        }

        // Bottom bar: Apply commits the draft into the live settings and persists them, without closing the
        // popup (only the "X" button above closes/cancels).
        auto bottom_bar = core::ecs::EntityRegistry::Create<core::ecs::entities::UI>("Entity::UI");
        auto &bottom_bar_layout = bottom_bar->getComponentMutable<core::components::Layout>();
        bottom_bar_layout.direction = core::components::Layout::Direction::Horizontal;
        bottom_bar_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        bottom_bar_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        bottom_bar_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 40.0f, 40.0f };
        bottom_bar_layout.child_alignment.horizontal = core::components::Layout::ChildAlignment::End;
        bottom_bar_layout.padding.top = 8;
        bottom_bar->setParent(*project_settings);

        auto apply_btn = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &apply_btn_rect = apply_btn->getComponentMutable<core::components::UIRect>();
        apply_btn_rect.color = core::types::Color("#3d8b40");
        apply_btn_rect.corner_radius = { 4, 4, 4, 4 };
        auto &apply_btn_layout = apply_btn->getComponentMutable<core::components::Layout>();
        apply_btn_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        apply_btn_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 100.0f, 100.0f };
        apply_btn_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::GROW;
        core::ecs::entities::UILabel(apply_btn->getChild("Button label")).setText("atmo.apply");
        apply_btn->setParent(*bottom_bar);

        apply_btn->getSignal<>("Released").connect([draft]() {
            project::ProjectManager::GetSettings() = *draft;
            project::ProjectManager::SaveSettings();
        });
    }
} // namespace atmo::editor

#endif
