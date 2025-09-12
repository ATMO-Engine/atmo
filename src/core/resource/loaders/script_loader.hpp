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
            class ScriptLoader : public ResourceLoader
            {
            public:
                ScriptLoader();
                ~ScriptLoader() = default;

                void load(const std::string &path);
                std::any get(const std::string &path);

            private:
                std::map<std::string, std::shared_ptr<char *>> _ressources;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
