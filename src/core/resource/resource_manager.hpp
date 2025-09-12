#pragma once

#include <any>
#include <map>
#include <string>
#include <vector>

#include "core/resource/resource_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ResourceManager
            {
            public:
                ResourceManager();
                ~ResourceManager() = default;

                std::any getResources(std::string &path);

            protected:
                std::map<std::string, std::unique_ptr<ResourceLoader>> _loaders;

            private:
                std::vector<std::string> split(const std::string &str, char delimiter);
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
