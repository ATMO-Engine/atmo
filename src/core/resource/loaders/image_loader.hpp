#pragma once

#include <memory>
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

                std::shared_ptr<SDL_Surface> load(const std::string &path) override;

                const std::string resourceTypeName() override;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
