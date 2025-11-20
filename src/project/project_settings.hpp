#pragma once

#include <semver.hpp>
#include <string>
#include "core/types.hpp"
#include "glaze/glaze.hpp"

#define VERSION_TYPE std::uint64_t
#define VERSION_TYPES VERSION_TYPE, VERSION_TYPE, VERSION_TYPE

#define ATMO_VERSION_MAJOR 1
#define ATMO_VERSION_MINOR 0
#define ATMO_VERSION_PATCH 0

#define ATMO_VERSION_STRING std::to_string(ATMO_VERSION_MAJOR) + "." + std::to_string(ATMO_VERSION_MINOR) + "." + std::to_string(ATMO_VERSION_PATCH)

#if defined(ATMO_EXPORT)
#define ATMO_SETTING const
#else
#define ATMO_SETTING
#endif

template <> struct glz::meta<semver::version<VERSION_TYPES>> {
    using T = semver::version<VERSION_TYPES>;
    static constexpr auto value = &T::to_string;
};

namespace atmo
{
    namespace project
    {
        struct App {
            ATMO_SETTING std::string project_name = "New Atmo Project";
            ATMO_SETTING semver::version<VERSION_TYPES> engine_version;
            ATMO_SETTING semver::version<VERSION_TYPES> project_version;
            ATMO_SETTING std::string icon_path = "res://atmo.png";
        };

        struct Boot {
            ATMO_SETTING atmo::core::types::rgba background_color = { 0, 0, 0, 255 };
            ATMO_SETTING bool show_splash_icon = true;
            ATMO_SETTING bool full_size_splash_icon = false;
            ATMO_SETTING std::string splash_icon_path = "res://atmo.png";
        };

        struct Singletons {
            ATMO_SETTING std::vector<std::string> scenes;
        };

        struct ProjectSettings {
            ATMO_SETTING App app;
            ATMO_SETTING Boot boot;
            ATMO_SETTING Singletons singletons;
            ATMO_SETTING std::string default_scene = "";
        };
    } // namespace project
} // namespace atmo
