#include "core/resource/pool.hpp"
#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "common/utils.hpp"
#include "core/resource/resource.hpp"
#include "core/resource/resource_factory.hpp"
#include "handle.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            Pool::Pool()
            {
                _usedHandles = {};
                _handles = {};
                _resources = {};
                _generations = {};
                _freeList = {};
            }

            Pool::~Pool() {}

            const Handle Pool::create(const std::string &path)
            {
                try {
                    if (_handles.find(path) != _handles.end()) {
                        return _handles.at(path);
                    }

                    std::string extension = atmo::common::Utils::splitString(path, '.').back();
                    ResourceFactory &fac = ResourceFactory::getInstance();
                    std::shared_ptr<Resource> res = fac.create(extension);
                    res->load(path);

                    Handle newHandle = {};
                    newHandle.frameToLive = 1; // TODO: Définir un nombre de frame durant lequel la ressource peut vivre
                                               // même sans être appelé
                    newHandle.path = path;

                    if (!_freeList.empty()) {
                        std::uint16_t idx = _freeList.back();
                        _freeList.pop_back();
                        _resources.at(idx) = res;
                        newHandle.generation = _generations.at(idx);
                        newHandle.index = idx;
                    } else {
                        newHandle.index = _resources.size();
                        newHandle.generation = 1;
                        _resources.push_back(res);
                        _generations.push_back(newHandle.generation);
                    }

                    _handles.insert(std::make_pair(path, newHandle));

                    return newHandle;
                } catch (const std::exception &e) {
                    throw e;
                }
            }

            std::shared_ptr<Resource> Pool::getFromHandle(const Handle &handle)
            {
                if (handle.generation != _generations.at(handle.index)) {
                    throw HandleOutDated("The handle is out dated");
                }
                return _resources.at(handle.index);
            }

            void Pool::declareHandle(const Handle &handle)
            {
                auto it = std::find_if(_usedHandles.begin(), _usedHandles.end(), [&handle](const Handle &h) {
                    return handle.path == h.path;
                });
                if (it != _usedHandles.end()) {
                    _usedHandles.push_back(handle);
                }
            }

            void Pool::destroy(const Handle &handle)
            {
                if (handle.generation != _generations.at(handle.index)) {
                    throw HandleOutDated("The handle is out dated");
                }
                _resources.at(handle.index)->destroy(); // TODO: Implementer avec le système de caching (retirer la
                                                        // ressource du vecteur et l'envoyer dans le cache)
                _generations.at(handle.index) += 1;
                _freeList.push_back(handle.index);
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
