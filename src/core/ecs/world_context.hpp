#pragma once

#include "SDL3/SDL_render.h"
#include "meta/meta.hpp"

namespace atmo::core::components
{
    struct WorldContext {
        bool is_editor_isolated = false;
        SDL_Renderer *renderer = nullptr;
        SDL_Texture *render_target = nullptr;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::WorldContext> {
    static constexpr const char *name = "WorldContext";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::WorldContext::is_editor_isolated>("is_editor_isolated"));
};
