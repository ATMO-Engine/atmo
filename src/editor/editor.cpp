#include "editor.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_checkbox/ui_checkbox.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "editor/editor_entities/ui_panel/ui_panel.hpp"
#include "glaze/json/prettify.hpp"
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
    Editor::Editor(atmo::core::Engine &engine, const std::string &project_path) : m_engine(engine), m_project_path(project_path) {}

    void Editor::registerDefaultCommands()
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

    void Editor::init()
    {
        registerDefaultCommands();

        auto scene = m_engine.getECS().getCurrentScene();

        m_menu_bar = makePlatformMenuBar();
        m_menu_bar->build(*scene, m_commands);

        auto white_rect = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &white_rect_rect = white_rect->getComponentMutable<core::components::UIRect>();
        white_rect_rect.color = core::types::Color::RED;
        white_rect_rect.color.a = 1.0f;
        // white_rect_rect.color.a = 0.0f;
        auto &white_rect_layout = white_rect->getComponentMutable<core::components::Layout>();
        white_rect_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        white_rect_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 1000.0f, 1200.0f };
        white_rect_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        white_rect_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 2000.0f, 2000.0f };
        white_rect_layout.direction = core::components::Layout::Direction::Vertical;
        white_rect_layout.padding = { 16, 0, 60, 8 };
        white_rect_layout.child_gap = 8;
        white_rect->rename("white rect");
        white_rect->setParent(*scene);


        auto button = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        button->getSignal<core::ecs::entities::UIButton &>("ToIdle").connect([](core::ecs::entities::UIButton &btn) {
            auto &rect = btn.getComponentMutable<core::components::UIRect>();
            rect.color = core::types::Color::GREEN;
            rect.color.a = 0.2f;

            rect.border.color = core::types::Color::BLACK;
        });
        button->getSignal<core::ecs::entities::UIButton &>("ToIdle").emit(*button);
        auto &button_layout = button->getComponentMutable<core::components::Layout>();
        button_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        button_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 320.0f, 320.0f };
        button_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        button_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 102.0f, 102.0f };
        button->rename("buttton");
        button->setParent(*white_rect);
        button->getSignal<core::ecs::entities::UIButton &>("Hover").connect([](core::ecs::entities::UIButton &btn) {
            auto &rect = btn.getComponentMutable<core::components::UIRect>();
            rect.color = core::types::Color{ static_cast<uint8_t>(155), static_cast<uint8_t>(255), static_cast<uint8_t>(200), static_cast<uint8_t>(255) };
        });
        button->getSignal<core::ecs::entities::UIButton &>("Pressed").connect([](core::ecs::entities::UIButton &btn) {
            auto &rect = btn.getComponentMutable<core::components::UIRect>();
            rect.color = core::types::Color{ static_cast<uint8_t>(135), static_cast<uint8_t>(55), static_cast<uint8_t>(255), static_cast<uint8_t>(255) };
        });
        button->getSignal<core::ecs::entities::UIButton &>("Released").connect([](core::ecs::entities::UIButton &btn) { spdlog::info("Button released"); });


        auto checkbox = core::ecs::EntityRegistry::Create<core::ecs::entities::UICheckBox>("Entity::UI::UIRect::UICheckBox");
        checkbox->getSignal<core::ecs::entities::UICheckBox &>("ToIdle").emit(*checkbox);

        auto &checkbox_layout = checkbox->getComponentMutable<core::components::Layout>();
        checkbox_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        checkbox_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 320.0f, 320.0f };
        checkbox_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        checkbox_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 102.0f, 102.0f };
        checkbox->rename("checkBox");
        checkbox->setParent(*white_rect);

        auto floating_window = core::ecs::EntityRegistry::Create<core::ecs::entities::UIFloatingWindow>("Entity::UI::UIRect::UIFloatingWindow");
        auto &floating_window_layout = floating_window->getComponentMutable<core::components::Layout>();
        floating_window_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        floating_window_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 320.0f, 320.0f };
        floating_window_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        floating_window_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 102.0f, 102.0f };
        floating_window->rename("floating window");
        floating_window->setParent(*checkbox);
        floating_window->getSignal<core::ecs::entities::UIFloatingWindow &>("Open").connect(
            [floating_window](core::ecs::entities::UIFloatingWindow &window) { floating_window->getComponentMutable<core::components::UI>().visible = true; });
        floating_window->getSignal<core::ecs::entities::UIFloatingWindow &>("Close").connect(
            [floating_window](core::ecs::entities::UIFloatingWindow &window) { floating_window->getComponentMutable<core::components::UI>().visible = false; });

        checkbox->getSignal<core::ecs::entities::UICheckBox &>("Clicked").connect([floating_window](core::ecs::entities::UICheckBox &chBox) {
            if (chBox.getComponent<core::components::UICheckBox>().trigger) {
                floating_window->getSignal<core::ecs::entities::UIFloatingWindow &>("Open").emit(*floating_window);
                auto parent = chBox.getParent<core::ecs::entities::UIRect>();
                auto &parent_layout = parent.getComponentMutable<core::components::Layout>();
                parent_layout.padding.left += 100;
            } else {
                floating_window->getSignal<core::ecs::entities::UIFloatingWindow &>("Close").emit(*floating_window);
                auto parent = chBox.getParent<core::ecs::entities::UIRect>();
                auto &parent_layout = parent.getComponentMutable<core::components::Layout>();
                parent_layout.padding.left -= 100;
            }
        });

        auto button1 = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        button1->getSignal<core::ecs::entities::UIButton &>("ToIdle").connect([](core::ecs::entities::UIButton &btn) {
            auto &rect = btn.getComponentMutable<core::components::UIRect>();
            rect.color = core::types::Color::GREEN;
            rect.color.a = 0.2f;

            rect.border.color = core::types::Color::BLACK;
        });
        button1->getSignal<core::ecs::entities::UIButton &>("ToIdle").emit(*button1);
        auto &button_layout1 = button1->getComponentMutable<core::components::Layout>();
        button_layout1.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        button_layout1.width.size = core::components::Layout::SizingAxis::MinMax{ 320.0f, 320.0f };
        button_layout1.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        button_layout1.height.size = core::components::Layout::SizingAxis::MinMax{ 102.0f, 102.0f };
        button1->rename("buttton1");
        button1->setParent(*white_rect);
        button1->getSignal<core::ecs::entities::UIButton &>("Hover").connect([](core::ecs::entities::UIButton &btn) {
            auto &rect = btn.getComponentMutable<core::components::UIRect>();
            rect.color = core::types::Color{ static_cast<uint8_t>(155), static_cast<uint8_t>(255), static_cast<uint8_t>(200), static_cast<uint8_t>(255) };
        });
        button1->getSignal<core::ecs::entities::UIButton &>("Pressed").connect([](core::ecs::entities::UIButton &btn) {
            auto &rect = btn.getComponentMutable<core::components::UIRect>();
            rect.color = core::types::Color{ static_cast<uint8_t>(135), static_cast<uint8_t>(55), static_cast<uint8_t>(255), static_cast<uint8_t>(255) };
        });
        button1->getSignal<core::ecs::entities::UIButton &>("Released").connect([](core::ecs::entities::UIButton &btn) { spdlog::info("Button released"); });


        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        label->setText("Hello, World!");
        label->setFontSize(48);
        label->rename("hello world");
        label->setParent(*floating_window);
        auto &label_layout = label->getComponentMutable<core::components::Layout>();

        spdlog::info(glz::write<glz::opts{ .prettify = true }>(scene->serialize()).value());
    }
} // namespace atmo::editor

#endif
