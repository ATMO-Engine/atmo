#pragma once

#include <map>
#include <string>
#include "core/types.hpp"
#include "glaze/glaze.hpp"
#include "impl/romver.hpp"
#include "meta/auto_register.hpp"
#include "meta/glaze_bridge.hpp"

#define ATMO_VERSION "1.0.0"

#define ATMO_PACKED_MAGIC_NUMBER 'A', 'T', 'M', 'O', '-', 'P', 'C', 'K'

#define ATMO_PROJECT_SETTING

namespace atmo
{
    namespace project
    {
        struct App {
            ATMO_PROJECT_SETTING std::string project_name = "New Atmo Project";
            ATMO_PROJECT_SETTING impl::Romver engine_version;
            ATMO_PROJECT_SETTING impl::Romver project_version;
            ATMO_PROJECT_SETTING std::string icon_path = "project://assets/atmo.png";
            ATMO_PROJECT_SETTING std::string default_scene = "";
        };

        struct Boot {
            ATMO_PROJECT_SETTING core::types::Color background_color = core::types::Color::BLACK;
            ATMO_PROJECT_SETTING bool show_splash_icon = true;
            ATMO_PROJECT_SETTING bool full_size_splash_icon = false;
            ATMO_PROJECT_SETTING std::string splash_icon_path = "project://assets/atmo.png";
        };

        struct Window {
            ATMO_PROJECT_SETTING core::types::Color background_color = core::types::Color("#191919");
            ATMO_PROJECT_SETTING bool headless = false;
            ATMO_PROJECT_SETTING core::types::Vector2i size = core::types::Vector2i{ 1280, 720 };
        };

        struct Singletons {
            ATMO_PROJECT_SETTING std::vector<std::string> scenes;
        };

        struct Engine {
            ATMO_PROJECT_SETTING core::types::Vector3 gravity{ 0.0f, 9.81f, 0.0f };
            ATMO_PROJECT_SETTING float physics_frame_rate = 60.0f;
            ATMO_PROJECT_SETTING float pixels_per_meter = 50.0f;
        };

        struct Debug {
            ATMO_PROJECT_SETTING bool draw_physics_debug = true;
        };

        struct Addons {
            ATMO_PROJECT_SETTING std::map<std::string, bool> addons;
        };

        struct ProjectSettings {
            ATMO_PROJECT_SETTING App app;
            ATMO_PROJECT_SETTING Boot boot;
            ATMO_PROJECT_SETTING Window window;
            ATMO_PROJECT_SETTING Singletons singletons;
            ATMO_PROJECT_SETTING Engine engine;
            ATMO_PROJECT_SETTING Debug debug;
            ATMO_PROJECT_SETTING Addons addons;
        };
    } // namespace project
} // namespace atmo

#if !defined(ATMO_EXPORT)

template <> struct atmo::meta::ComponentMeta<atmo::project::App> {
    static constexpr const char *name = "ProjectSettings.App";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::project::App::project_name>("project_name"),
        atmo::meta::field<&atmo::project::App::icon_path>("icon_path").withWidget("file_path"),
        atmo::meta::field<&atmo::project::App::default_scene>("default_scene").withWidget("file_path"));
};
ATMO_REGISTER_COMPONENT_NO_FLECS(atmo::project::App);

template <> struct atmo::meta::ComponentMeta<atmo::project::Boot> {
    static constexpr const char *name = "ProjectSettings.Boot";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::project::Boot::background_color>("background_color"),
        atmo::meta::field<&atmo::project::Boot::show_splash_icon>("show_splash_icon"),
        atmo::meta::field<&atmo::project::Boot::full_size_splash_icon>("full_size_splash_icon"),
        atmo::meta::field<&atmo::project::Boot::splash_icon_path>("splash_icon_path").withWidget("file_path"));
};
ATMO_REGISTER_COMPONENT_NO_FLECS(atmo::project::Boot);

template <> struct atmo::meta::ComponentMeta<atmo::project::Window> {
    static constexpr const char *name = "ProjectSettings.Window";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::project::Window::background_color>("background_color"), atmo::meta::field<&atmo::project::Window::headless>("headless"),
        atmo::meta::field<&atmo::project::Window::size>("size"));
};
ATMO_REGISTER_COMPONENT_NO_FLECS(atmo::project::Window);

template <> struct atmo::meta::ComponentMeta<atmo::project::Singletons> {
    static constexpr const char *name = "ProjectSettings.Singletons";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::project::Singletons::scenes>("scenes"));
};
ATMO_REGISTER_COMPONENT_NO_FLECS(atmo::project::Singletons);

template <> struct atmo::meta::ComponentMeta<atmo::project::Engine> {
    static constexpr const char *name = "ProjectSettings.Engine";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::project::Engine::gravity>("gravity"), atmo::meta::field<&atmo::project::Engine::physics_frame_rate>("physics_frame_rate"),
        atmo::meta::field<&atmo::project::Engine::pixels_per_meter>("pixels_per_meter"));
};
ATMO_REGISTER_COMPONENT_NO_FLECS(atmo::project::Engine);

template <> struct atmo::meta::ComponentMeta<atmo::project::Debug> {
    static constexpr const char *name = "ProjectSettings.Debug";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::project::Debug::draw_physics_debug>("draw_physics_debug"));
};
ATMO_REGISTER_COMPONENT_NO_FLECS(atmo::project::Debug);

#endif
