#pragma once

#include <SDL3_ttf/SDL_ttf.h>

#include "core/resource/resource.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class FontLoader : public Resource<TTF_Font>
            {
            public:
                FontLoader();
                ~FontLoader() override;

                TTF_Font *load(const std::string &path) override;
                void destroy(TTF_Font *res) override;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
