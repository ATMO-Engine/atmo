#pragma once

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
                ResourceLoader() = delete;

                // TODO smart pointer resource system
                void get(const std::string &path);
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
