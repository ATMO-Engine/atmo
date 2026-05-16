#pragma once

#include <map>
#include <string>
#include "core/types.hpp"
#include "glaze/glaze.hpp"
#include "impl/romver.hpp"

#define ATMO_VERSION "1.0.0"

#define ATMO_PACKED_MAGIC_NUMBER 'A', 'T', 'M', 'O', '-', 'P', 'C', 'K'

#if defined(ATMO_EXPORT)
#define ATMO_PROJECT_SETTING const
#else
#define ATMO_PROJECT_SETTING
#endif

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
            ATMO_PROJECT_SETTING core::types::Color background_color = core::types::Color::BLACK;
            ATMO_PROJECT_SETTING bool headless = false;
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
