#pragma once

#include "SDL3/SDL_render.h"
#include "core/types.hpp"
#include "meta/meta.hpp"
#include "server/rendering/rid.hpp"

namespace atmo::core::components
{
    struct WorldContext {
        bool is_editor_isolated = false;
        SDL_Renderer *renderer = nullptr;
        SDL_Texture *render_target = nullptr;
        server::rendering::CanvasRid canvas;
        server::rendering::ViewportRid viewport;
        types::Vector2 viewport_size{ 0.0f, 0.0f };
        types::Vector2 viewport_dpi{ 1.0f, 1.0f };
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::WorldContext> {
    static constexpr const char *name = "WorldContext";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::WorldContext::is_editor_isolated>("is_editor_isolated"));
};
