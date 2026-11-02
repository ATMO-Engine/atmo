#pragma once

#pragma clang attribute push(__attribute__((no_sanitize("null"))), apply_to = function)
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#pragma clang attribute pop
