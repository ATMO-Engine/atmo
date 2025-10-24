#pragma once

#include <semver.hpp>
#include <string>
#include "core/types.hpp"

#define VERSION_TYPE std::uint64_t
#define VERSION_TYPES VERSION_TYPE, VERSION_TYPE, VERSION_TYPE

#define ATMO_VERSION_MAJOR 0
#define ATMO_VERSION_MINOR 1
#define ATMO_VERSION_PATCH 0

#define ATMO_VERSION_STRING std::to_string(ATMO_VERSION_MAJOR) + "." + std::to_string(ATMO_VERSION_MINOR) + "." + std::to_string(ATMO_VERSION_PATCH)

#if defined(ATMO_EXPORT)
#define ATMO_SETTING const
#else
#define ATMO_SETTING
#endif

struct App {
    ATMO_SETTING char project_name[64] = { 0 };
    ATMO_SETTING semver::version<VERSION_TYPES> engine_version;
    ATMO_SETTING semver::version<VERSION_TYPES> project_version;
    ATMO_SETTING char icon_path[256] = { 0 };
};

struct Boot {
    // ATMO_SETTING atmo::core::types::rgba backgroundColor;
    ATMO_SETTING bool show_splash_icon = true;
    ATMO_SETTING bool full_size_splash_icon = false;
    ATMO_SETTING char splash_icon_path[256] = { 0 };
};

struct ProjectSettings {
    ATMO_SETTING char atmo_signature[4] = { 'A', 'T', 'M', 'O' };
    ATMO_SETTING App app;
    ATMO_SETTING Boot boot;
    ATMO_SETTING char default_scene[256] = { 0 };
};
