#include "editor.hpp"
#include "core/ecs/entities/ui/ui.hpp"
#include "core/ecs/entities/ui/ui_label/ui_label.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "glaze/json/prettify.hpp"
#include "project/file_system.hpp"
#include "spdlog/spdlog.h"

#if !defined(ATMO_EXPORT)

#include "SDL3/SDL_keycode.h"
#include "core/ecs/ecs.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/ui/ui_rect/ui_rect.hpp"

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

        // auto white_rect = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        // auto &white_rect_rect = white_rect->getComponentMutable<core::components::UIRect>();
        // auto &white_rect_layout = white_rect->getComponentMutable<core::components::Layout>();
        // white_rect_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        // white_rect_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        // white_rect_layout.padding = { 8, 8, 8, 8 };
        // white_rect_layout.child_gap = 8;
        // white_rect->rename("white rect");
        // white_rect->setParent(*scene);

        // auto red_rect = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        // auto &red_rect_rect = red_rect->getComponentMutable<core::components::UIRect>();
        // red_rect_rect.color = core::types::Color::RED;
        // auto &red_rect_layout = red_rect->getComponentMutable<core::components::Layout>();
        // red_rect_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        // red_rect_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 100.0f, 100.0f };
        // red_rect_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        // red_rect_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 100.0f, 100.0f };
        // red_rect->rename("red rect");
        // red_rect->setParent(*white_rect);

        // auto green_rect = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        // auto &green_rect_rect = green_rect->getComponentMutable<core::components::UIRect>();
        // green_rect_rect.color = core::types::Color::GREEN;
        // auto &green_rect_layout = green_rect->getComponentMutable<core::components::Layout>();
        // green_rect_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        // green_rect_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 100.0f, 100.0f };
        // green_rect_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        // green_rect_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 100.0f, 100.0f };
        // green_rect->rename("green rect");
        // green_rect->setParent(*white_rect);

        // auto blue_rect = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        // auto &blue_rect_rect = blue_rect->getComponentMutable<core::components::UIRect>();
        // blue_rect_rect.color = core::types::Color::BLUE;
        // auto &blue_rect_layout = blue_rect->getComponentMutable<core::components::Layout>();
        // blue_rect_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        // blue_rect_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 100.0f, 100.0f };
        // blue_rect_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        // blue_rect_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 100.0f, 100.0f };
        // blue_rect->rename("blue rect");
        // blue_rect->setParent(*white_rect);

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
