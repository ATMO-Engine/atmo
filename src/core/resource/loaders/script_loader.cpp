#include <exception>
#include <fstream>
#include <memory>
#include <stdexcept>

#include "core/resource/loaders/script_loader.hpp"
#include "luau/luau.hpp"
#include "script_loader.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ScriptLoader::ScriptLoader() {}

            ScriptLoader::~ScriptLoader() {}

            std::shared_ptr<Bytecode> ScriptLoader::load(const std::string &path)
            {
                try {
                    std::ifstream luaFile(path);
                    if (!luaFile) {
                        throw std::runtime_error("Failed to open script file: " + path);
                    }

                    std::string source((std::istreambuf_iterator<char>(luaFile)), std::istreambuf_iterator<char>());
                    luaFile.close();

                    size_t bytecodeSize = 0;
                    char *bytecode = atmo::luau::Luau::Compile(source, &bytecodeSize);

                    Bytecode *newRessource = new Bytecode{};
                    newRessource->data = bytecode;
                    newRessource->size = bytecodeSize;
                    return std::shared_ptr<Bytecode>(
                        newRessource,
                        [](Bytecode *b) {
                            if (b) {
                                if (b->data) {
                                    free(b->data);
                                }
                                delete b;
                            }
                        }
                    );
                }
                catch (const std::exception &e) {
                    throw e;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
