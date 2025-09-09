#pragma once

#include <any>
#include <map>
#include <vector>
#include "flecs.h"
#include "glaze/glaze.hpp"
#include "luau/luau.hpp"

namespace atmo
{
    namespace core
    {
        class Scene
        {
        public:
            void load_from_bytes(const char *bytes, size_t size);
            char *save_to_bytes(size_t *size);
        };
    } // namespace core
} // namespace atmo
