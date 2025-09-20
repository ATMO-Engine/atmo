#include <any>
#include <exception>
#include <fstream>

#include "core/resource/loaders/script_pool.hpp"
#include "core/resource/handle.hpp"
#include "luau/luau.hpp"


namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ScriptPool::ScriptPool() {}

            ScriptPool::~ScriptPool()
            {
                for (auto elm : _resources) {
                    if (elm.data != nullptr) {
                        free(elm.data);
                    }
                }
            }

            const Handle ScriptPool::create(const std::string &path)
            {
                try {
                    std::ifstream luaFile(path);

                    std::string source((std::istreambuf_iterator<char>(luaFile)), std::istreambuf_iterator<char>());
                    luaFile.close();

                    size_t bytecodeSize = 0;
                    char *bytecode = atmo::luau::Luau::compile(source, &bytecodeSize);

                    Bytecode newRessource;
                    newRessource.data = bytecode;
                    newRessource.size = bytecodeSize;

                    Handle newHandle = {};
                    newHandle.frameToLive = 1; // TODO: Définir un nombre de frame durant lequel la ressource peut vivre même sans être appelé
                    newHandle.type = ResourceType::Script;

                    if (!_freeList.empty()) {
                        std::uint16_t idx = _freeList.back();
                        _freeList.pop_back();
                        _resources.at(idx) = newRessource;
                        newHandle.generation = _generations.at(idx);
                        newHandle.index = idx;
                    } else {
                        newHandle.index = _resources.size();
                        newHandle.generation = 1;
                        _resources.push_back(newRessource);
                        _generations.push_back(newHandle.generation);
                    }

                    return newHandle;
                }
                catch (const std::exception &e) {
                    throw e;
                }
            }

            std::any ScriptPool::getFromHandle(const Handle &handle)
            {
                if (handle.generation != _generations.at(handle.index)) {
                    throw std::exception("Handle périmé");
                }
                return std::make_any<Bytecode>(_resources.at(handle.index));
            }

            void ScriptPool::destroy(const Handle &handle)
            {
                free(_resources.at(handle.index).data); // TODO: Implementer avec le système de caching (retirer la ressource du vecteur et l'envoyer dans le cache)
                _resources.at(handle.index).data = nullptr;
                _generations.at(handle.index) += 1;
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
