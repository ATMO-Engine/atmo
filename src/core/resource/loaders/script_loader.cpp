#include "core/resource/loaders/script_loader.hpp"
#include <any>
#include <fstream>
#include <memory>
#include "luau/luau.hpp"


namespace atmo
{
    namespace core
    {
        namespace resource
        {
            void ScriptLoader::load(const std::string &path)
            {
                size_t bytecodeSize = 0;
                char *bytecode = atmo::luau::Luau::compile(path, &bytecodeSize);

                std::ofstream out("compiled.luac", std::ios::binary);
                if (out.is_open()) {
                    out.write(bytecode, bytecodeSize);
                    out.close();
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
g
