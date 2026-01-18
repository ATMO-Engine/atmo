#pragma once

#include <memory>
#include "core/resource/resource.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            struct Bytecode
            {
                char *data;
                size_t size;
            };

            class ScriptLoader : public Resource<Bytecode>
            {
            public:
                ScriptLoader();
                ~ScriptLoader() override;

                std::shared_ptr<Bytecode> load(const std::string &path) override;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
