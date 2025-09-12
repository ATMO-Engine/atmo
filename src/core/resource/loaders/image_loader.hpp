#pragma once

#include <any>
#include <map>
#include <memory>
#include "core/resource/resource_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ImageLoader : public ResourceLoader
            {
            public:
                ImageLoader();
                ~ImageLoader() = default;

                void load(const std::string &path);
                std::any get(const std::string &path);

            private:
                std::map<std::string, std::shared_ptr<std::string>> _ressources;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
