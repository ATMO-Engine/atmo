#pragma once

#include "core/resource/resource.hpp"

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
                std::string _texture;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
