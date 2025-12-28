#pragma once

#include "core/resource/resource.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ImageLoader : public Resource<std::string>
            {
            public:
                ImageLoader();
                ~ImageLoader() override;

                std::string *load(const std::string &path) override;
                void destroy(std::string *res) override;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
