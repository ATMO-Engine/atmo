#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <memory>

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

                std::shared_ptr<TTF_Font> load(const std::string &path) override;

                const std::string resourceTypeName() override;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
