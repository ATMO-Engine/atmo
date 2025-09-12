#pragma once

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
                std::shared_ptr<void> &get(const std::string &path);
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
