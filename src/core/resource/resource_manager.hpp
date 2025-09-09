#pragma once

#include <map>
#include <string>

#include "core/resource/loaders/image_loader.hpp"
#include "core/resource/resource_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ResourceManager
            {
            protected:
                static const std::map<std::string, ResourceLoader> _loaders;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
