#pragma once

#include "SDL3/SDL_render.h"
#include "core/resource/resource.hpp"
#include "core/resource/resource_register.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ImageLoader : public Resource
            {
            public:
                ImageLoader();
                ~ImageLoader() override;

                void load(const std::string &path) override;
                void destroy() override;

                std::any get() override;

            private:
                SDL_Surface *m_surface;

                static LoaderRegister<ImageLoader> m_register;
            };

            template <> struct LoaderExtension<ImageLoader> {
                static constexpr const char *extension = "png";
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
