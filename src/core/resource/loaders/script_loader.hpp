#pragma once

#include "core/resource/resource_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ScriptLoader : public ResourceLoader
            {
            public:
                ScriptLoader();
                ~ScriptLoader() = default;

                void load(const std::string &path);
                std::any get(const std::string &path);
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
