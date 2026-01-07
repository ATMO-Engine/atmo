#include <exception>
#include <fstream>

#include "core/resource/loaders/script_loader.hpp"
#include "luau/luau.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ScriptLoader::ScriptLoader() {}

            ScriptLoader::~ScriptLoader() {}

            Bytecode *ScriptLoader::load(const std::string &path)
            {
                try {
                    std::ifstream luaFile(path);

                    std::string source((std::istreambuf_iterator<char>(luaFile)), std::istreambuf_iterator<char>());
                    luaFile.close();

                    size_t bytecodeSize = 0;
                    char *bytecode = atmo::luau::Luau::Compile(source, &bytecodeSize);

                    Bytecode *newRessource = new Bytecode{};
                    newRessource->data = bytecode;
                    newRessource->size = bytecodeSize;
                    return newRessource;
                }
                catch (const std::exception &e) {
                    throw e;
                }
            }

            void ScriptLoader::destroy(Bytecode *res)
            {
                if (res->data != nullptr) {
                    free(res->data); // TODO: Implementer avec le système de caching (retirer la ressource du vecteur et l'envoyer dans le cache)
                }
                res->data = nullptr;
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
