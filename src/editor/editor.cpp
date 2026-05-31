#include "editor.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_button/ui_button.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
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
        white_rect_rect.color = core::types::Color::WHITE;
        // white_rect_rect.color.a = 0.0f;
        auto &white_rect_layout = white_rect->getComponentMutable<core::components::Layout>();
        white_rect_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        white_rect_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 500.0f, 320.0f };
        white_rect_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        white_rect_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 2000.0f, 2000.0f };
        white_rect_layout.direction = core::components::Layout::Direction::Vertical;
        white_rect_layout.padding = { 16, 0, 60, 8 };
        white_rect_layout.child_gap = 8;
        white_rect->rename("white rect");
        white_rect->setParent(*scene);
        auto red_rect = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &red_rect_rect = red_rect->getComponentMutable<core::components::UIRect>();
        red_rect_rect.color = core::types::Color::RED;
        auto &red_rect_layout = red_rect->getComponentMutable<core::components::Layout>();
        red_rect_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        red_rect_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 320.0f, 320.0f };
        red_rect_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        red_rect_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 1029.0f, 1029.0f };
        // red_rect_layout.padding = { 16, 16, 60, 16 };
        red_rect->rename("red rect");
        red_rect->setParent(*white_rect);


        auto button = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        auto &button_rect = button->getComponentMutable<core::components::UIRect>();
        button_rect.color = core::types::Color::GREEN;
        button_rect.color.a = 0.2f;
        auto &button_layout = button->getComponentMutable<core::components::Layout>();
        button_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        button_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 320.0f, 320.0f };
        button_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        button_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 102.0f, 102.0f };
        // button_layout.padding = { 16, 16, 60, 16 };
        button->rename("buttton");
        button->setParent(*scene);

        // auto button1 = core::ecs::EntityRegistry::Create<core::ecs::entities::UIButton>("Entity::UI::UIRect::UIButton");
        // auto &button_rect1 = button1->getComponentMutable<core::components::UIRect>();
        // button_rect1.color = core::types::Color::GREEN;
        // button_rect1.color.a = 0.2f;
        // auto &button_layout1 = button1->getComponentMutable<core::components::Layout>();
        // button_layout1.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        // button_layout1.width.size = core::components::Layout::SizingAxis::MinMax{ 320.0f, 320.0f };
        // button_layout1.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        // button_layout1.height.size = core::components::Layout::SizingAxis::MinMax{ 102.0f, 102.0f };
        // button1->rename("buttton1");
        // button1->setParent(*scene);


        auto label = core::ecs::EntityRegistry::Create<core::ecs::entities::UILabel>("Entity::UI::UILabel");
        label->setFontPath("project://assets/fonts/Nunito/Nunito.ttf");
        label->setText("Hello, World!");
        label->setFontSize(48);
        label->rename("hello world");
        label->setParent(*scene);
        auto &label_layout = label->getComponentMutable<core::components::Layout>();

        spdlog::info(glz::write<glz::opts{ .prettify = true }>(scene->serialize()).value());
    }
} // namespace atmo::editor

#endif
