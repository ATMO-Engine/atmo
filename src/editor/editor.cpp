#include "editor.hpp"
#include "core/ecs/entities/ui/ui_layout.hpp"
#include "core/ecs/entity_registry.hpp"
#include "glaze/json/prettify.hpp"
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

        auto rect = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &rect_rect = rect->getComponentMutable<core::components::UIRect>();
        auto &rect_layout = rect->getComponentMutable<core::components::Layout>();
        rect_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        rect_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIT;
        rect->rename("white rect");
        rect->setParent(*scene);

        auto rect1 = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &rect1_rect = rect1->getComponentMutable<core::components::UIRect>();
        rect1_rect.color = { 255, 0, 0, 255 };
        auto &rect1_layout = rect1->getComponentMutable<core::components::Layout>();
        rect1_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        rect1_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 50.0f, 50.0f };
        rect1_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        rect1_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 50.0f, 50.0f };
        rect1->rename("red rect");
        rect1->setParent(*rect);

        auto rect2 = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &rect2_rect = rect2->getComponentMutable<core::components::UIRect>();
        rect2_rect.color = { 0, 255, 0, 255 };
        auto &rect2_layout = rect2->getComponentMutable<core::components::Layout>();
        rect2_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        rect2_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 50.0f, 50.0f };
        rect2_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        rect2_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 50.0f, 50.0f };
        rect2->rename("green rect");
        rect2->setParent(*rect);

        auto rect3 = core::ecs::EntityRegistry::Create<core::ecs::entities::UIRect>("Entity::UI::UIRect");
        auto &rect3_rect = rect3->getComponentMutable<core::components::UIRect>();
        rect3_rect.color = { 0, 0, 255, 255 };
        auto &rect3_layout = rect3->getComponentMutable<core::components::Layout>();
        rect3_layout.width.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        rect3_layout.width.size = core::components::Layout::SizingAxis::MinMax{ 50.0f, 50.0f };
        rect3_layout.height.type = core::components::Layout::SizingAxis::SizingAxisType::FIXED;
        rect3_layout.height.size = core::components::Layout::SizingAxis::MinMax{ 50.0f, 50.0f };
        rect3->rename("blue rect");
        rect3->setParent(*rect);

        spdlog::info(glz::write<glz::opts{ .prettify = true }>(scene->serialize())->c_str());
    }
} // namespace atmo::editor

#endif
