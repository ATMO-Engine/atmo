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
            LoaderRegister<ScriptLoader> ScriptLoader::_register("luau");

            ScriptLoader::ScriptLoader() {}

            ScriptLoader::~ScriptLoader()
            {
                if (m_script.data != nullptr) {
                    free(m_script.data);
                }
            }

            void ScriptLoader::load(const std::string &path)
            {
                try {
                    std::ifstream luaFile(path);

                    std::string source((std::istreambuf_iterator<char>(luaFile)), std::istreambuf_iterator<char>());
                    luaFile.close();

                    size_t bytecodeSize = 0;
                    char *bytecode = atmo::luau::Luau::Compile(source, &bytecodeSize);

                    Bytecode newRessource;
                    newRessource.data = bytecode;
                    newRessource.size = bytecodeSize;
                } catch (const std::exception &e) {
                    throw e;
                }
            }

            std::any ScriptLoader::get()
            {
                return std::make_any<Bytecode>(m_script);
            }

            void ScriptLoader::destroy()
            {
                if (m_script.data != nullptr) {
                    free(m_script.data); // TODO: Implementer avec le système de caching (retirer la ressource du vecteur et l'envoyer dans le cache)
                }
                m_script.data = nullptr;
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
