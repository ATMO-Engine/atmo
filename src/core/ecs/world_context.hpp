#pragma once

#include "SDL3/SDL_render.h"
#include "core/event/event_registry.hpp"
#include "core/event/events/event.hpp"
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

#if !defined(ATMO_EXPORT)
namespace atmo::editor
{
    class ProgressTickEvent : public core::event::EventRegistry::Registrable<ProgressTickEvent, core::event::events::Event>
    {
    public:
        using core::event::EventRegistry::Registrable<ProgressTickEvent, core::event::events::Event>::Registrable;

        static constexpr std::string_view LocalName()
        {
            return "ProgressTickEvent";
        }

        float delta_time = 0.0f;
    };
} // namespace atmo::editor
#endif
