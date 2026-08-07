#pragma once

#include <memory>
#include "SDL3/SDL_render.h"
#include "core/resource/resource.hpp"

namespace atmo::core::resource
{
    class TextureLoader : public ContextualResource<SDL_Texture, SDL_Renderer *>
    {
    public:
        TextureLoader();
        ~TextureLoader() override;

        std::shared_ptr<SDL_Texture> loadWithContext(const std::string &path, SDL_Renderer *renderer) override;
    };
} // namespace atmo::core::resource
