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

            class ScriptLoader : public Resource
            {
            public:
                ScriptLoader();
                ~ScriptLoader() override;

                void load(const std::string &path) override;
                void destroy() override;

                std::any get() override;
            private:
                Bytecode _script;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
