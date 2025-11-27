#pragma once

#include <SDL3_ttf/SDL_ttf.h>

#include "core/resource/resource.hpp"
#include "core/resource/resource_register.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class FontLoader : public Resource
            {
            public:
                FontLoader();
                ~FontLoader() override;

                void load(const std::string &path) override;
                void destroy() override;

                std::any get() override;

            private:
                TTF_Font *m_font;

                static LoaderRegister<FontLoader> _register;
            };

            template <> struct LoaderExtension<FontLoader> {
                static constexpr const char *extension = "ttf";
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
