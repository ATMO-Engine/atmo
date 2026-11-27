#if !defined(ATMO_EXPORT)

#include "app_icon.hpp"

#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_surface.h"
#include "SDL3_image/SDL_image.h"
#include "core/ecs/entities/window/window.hpp"
#include "project/project_settings.hpp"
#include "spdlog/spdlog.h"

namespace atmo::editor
{
#if !defined(__APPLE__)
    namespace
    {
        constexpr unsigned char app_icon_png[] = {
#include "atmo-256.png.h"
        };
    } // namespace
#endif

    void SetAppMetadata()
    {
        SDL_SetAppMetadata("Atmo", ATMO_VERSION, "atmo");
    }

    bool ApplyAppIcon(core::ecs::entities::Window &window)
    {
#if defined(__APPLE__)
        (void)window;
        return false;
#else
        SDL_IOStream *stream = SDL_IOFromConstMem(app_icon_png, sizeof(app_icon_png));
        if (!stream) {
            spdlog::warn("Failed to read the editor icon: {}", SDL_GetError());
            return false;
        }

        SDL_Surface *icon = IMG_Load_IO(stream, true);
        if (!icon) {
            spdlog::warn("Failed to decode the editor icon: {}", SDL_GetError());
            return false;
        }

        const bool applied = window.setIcon(icon);
        SDL_DestroySurface(icon);
        return applied;
#endif
    }
} // namespace atmo::editor

#endif
