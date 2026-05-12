#pragma once

#include <spdlog/spdlog.h>

#include "addon/addon.hpp"
#include "core/args/arg_manager.hpp"
#include "core/engine.hpp"
#include "project/file_system.hpp"
#include "project/project_manager.hpp"

#define TR(key) atmo::locale::LocaleManager::GetTranslation(key)
