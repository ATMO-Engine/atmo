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
                virtual ~ResourceLoader() = default;
                virtual void load(const std::string &path) = 0;
                virtual std::any get(const std::string &path) = 0;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
