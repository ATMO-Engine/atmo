#pragma once

#include "core/resource/resource.hpp"
#include "core/resource/resource_register.hpp"

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

                static LoaderRegister<ScriptLoader> _register;
            };

            template<>
            struct LoaderExtension<ScriptLoader> {
                static constexpr const char *extension = "luau";
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
