#pragma once

#include "SDL3/SDL_render.h"
#include "core/resource/resource.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ImageLoader : public Resource<SDL_Surface>
            {
            public:
                ImageLoader();
                ~ImageLoader() override;

                SDL_Surface *load(const std::string &path) override;
                void destroy(SDL_Surface *res) override;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
