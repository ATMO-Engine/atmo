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
            Scene();

            typedef struct Script
            {
                std::string file;
                atmo::luau::Luau luau;
            } Script;

            void load_from_bytes(const char *bytes, size_t size);
            char *save_to_bytes(size_t *size);

        protected:
            flecs::world ecs;
            std::map<std::string, flecs::entity> prefabs;
        };
    } // namespace core
} // namespace atmo
