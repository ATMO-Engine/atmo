#pragma once

#include <any>
#include <memory>
#include <string>

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ResourceLoader
            {
            public:
                // TODO smart pointer resource system
                void load(const std::string &path);
                std::any &get(const std::string &path);
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
