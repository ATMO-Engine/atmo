#include "core/resource/loaders/script_loader.hpp"
#include <any>
#include <fstream>
#include <iostream>
#include "luau/luau.hpp"


namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ScriptLoader::ScriptLoader() {}

            void ScriptLoader::load(const std::string &path)
            {
                try {
                    std::ifstream luaFile(path);

                    std::string source((std::istreambuf_iterator<char>(luaFile)), std::istreambuf_iterator<char>());
                    luaFile.close();

                    size_t bytecodeSize = 0;
                    char *bytecode = atmo::luau::Luau::compile(source, &bytecodeSize);

                    std::ofstream out("compiled.luac", std::ios::binary);
                    if (out.is_open()) {
                        out.write(bytecode, bytecodeSize);
                        out.close();
                    }
                }
                catch (const std::exception &e) {
                    throw e;
                }
            }
            std::any ScriptLoader::get(const std::string &path) {}
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
