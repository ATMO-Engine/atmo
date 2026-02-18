#pragma once

#include <string>
#include "core/types.hpp"
#include "impl/romver.hpp"
#include "glaze/glaze.hpp"

#define VERSION_TYPE std::uint64_t
#define VERSION_TYPES VERSION_TYPE, VERSION_TYPE, VERSION_TYPE

#define ATMO_VERSION_PROJECT 0
#define ATMO_VERSION_MAJOR 1
#define ATMO_VERSION_MINOR 0

#define ATMO_VERSION_STRING std::to_string(ATMO_VERSION_PROJECT) + "." + std::to_string(ATMO_VERSION_MAJOR) + "." + std::to_string(ATMO_VERSION_MINOR)

#define ATMO_PACKED_MAGIC_NUMBER 'A', 'T', 'M', 'O', '-', 'P', 'C', 'K'

#if defined(ATMO_EXPORT)
#define ATMO_SETTING const
#else
#define ATMO_SETTING
#endif

template <> struct glz::meta<atmo::core::types::rgba> {
    using T = atmo::core::types::rgba;
    static constexpr auto value = glz::object("r", &T::r, "g", &T::g, "b", &T::b, "a", &T::a);
};

namespace atmo
{
    namespace project
    {
        struct App {
            ATMO_SETTING std::string project_name = "New Atmo Project";
            ATMO_SETTING std::string engine_version;
            ATMO_SETTING std::string project_version;
            ATMO_SETTING std::string icon_path = "project://assets/atmo.png";
        };

        struct Boot {
            ATMO_SETTING atmo::core::types::rgba background_color = { 0, 0, 0, 255 };
            ATMO_SETTING bool show_splash_icon = true;
            ATMO_SETTING bool full_size_splash_icon = false;
            ATMO_SETTING std::string splash_icon_path = "project://assets/atmo.png";
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
