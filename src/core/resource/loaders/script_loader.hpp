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

            struct Bytecode
            {
                std::shared_ptr<char *> data;
                size_t size;
            };

            class ScriptLoader : public ResourceLoader
            {
            public:
                ScriptLoader();
                ~ScriptLoader() = default;

                void load(const std::string &path);
                std::any get(const std::string &path);

            private:
                std::map<std::string, Bytecode> _ressources;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
