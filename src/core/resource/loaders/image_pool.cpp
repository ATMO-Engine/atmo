#include "core/resource/loaders/image_pool.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ImagePool::ImagePool() {}

            const Handle ImagePool::create(const std::string &path)
            {
                try {
                    std::string newRessource = std::string("test string loaded");

                    Handle newHandle = {};
                    newHandle.frameToLive = 1; // TODO: Définir un nombre de frame durant lequel la ressource peut vivre même sans être appelé
                    newHandle.type = ResourceType::PNG;

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

            std::any ImagePool::getFromHandle(const Handle &handle)
            {
                if (handle.generation != _generations.at(handle.index)) {
                    throw std::exception("Handle périmé");
                }
                return std::make_any<std::string>(_resources.at(handle.index));
            }

            void ImagePool::destroy(const Handle &handle)
            {
                _resources.at(handle.index).clear();
                _generations.at(handle.index) += 1;
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
