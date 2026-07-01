#pragma once

#include <memory>
#include "SDL3/SDL_render.h"
#include "core/resource/resource.hpp"

namespace atmo::core::resource
{
    class TextureLoader : public Resource<SDL_Texture>
    {
    public:
        TextureLoader();
        ~TextureLoader() override;

        std::shared_ptr<SDL_Texture> load(const std::string &path) override;

        const std::string resourceTypeName() override;
    };
} // namespace atmo::core::resource
