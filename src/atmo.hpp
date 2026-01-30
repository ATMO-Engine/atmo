#pragma once

#include <spdlog/spdlog.h>

#include "core/args/arg_manager.hpp"
#include "core/engine.hpp"
#include "project/file_system.hpp"

#define TR(key) atmo::locale::LocaleManager::GetTranslation(key)
