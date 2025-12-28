#pragma once

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

                Bytecode *load(const std::string &path) override;
                void destroy(Bytecode *res) override;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
