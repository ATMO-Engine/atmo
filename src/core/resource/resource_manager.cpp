#include "resource_manager.hpp"
#include <exception>
#include <memory>
#include "common/utils.hpp"
#include "core/resource/resource_factory.hpp"

#include "core/resource/loaders/image_loader.hpp"
#include "core/resource/loaders/script_loader.hpp"
#include "loaders/font_loader.hpp"
#include "resource_register.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            using LoaderTypes = std::tuple<ImageLoader, ScriptLoader, FontLoader>;

            ResourceManager::ResourceManager() : _factory(ResourceFactory::getInstance())
            {
                _pools = makePoolMap<LoaderTypes>();
            }

            ResourceManager &ResourceManager::getInstance()
            {
                static ResourceManager instance;
                return instance;
            }


            const Handle ResourceManager::generate(const std::string &path)
            {
                std::string extension = atmo::common::Utils::splitString(path, '.').back();
                try {
                    if (_pools.find(extension) != _pools.end()) {
                        Handle newHandle = _pools.at(extension).create(path);
                        return newHandle;
                    } else {
                        throw InvalidLoader("No loader found for " + extension + " file");
                    }
                } catch (const std::exception &e) {
                    throw e;
                }
            }

            std::shared_ptr<Resource> ResourceManager::getResource(const Handle &handle)
            {
                std::string extension = atmo::common::Utils::splitString(handle.path, '.').back();
                try {
                    if (_pools.find(extension) != _pools.end()) {
                        // create Resource class through a calss that return a Resource class thanks to the path
                        return _pools.at(extension).getFromHandle(handle);
                    } else {
                        throw InvalidLoader("No loader found for " + extension + " file");
                    }
                } catch (const std::exception &e) {
                    throw e;
                }
            }

            void ResourceManager::declareHandle(const Handle &handle)
            {
                std::string extension = atmo::common::Utils::splitString(handle.path, '.').back();
                try {
                    if (_pools.find(extension) != _pools.end()) {
                        // create Resource class through a class that return a Resource class thanks to the path
                        _pools.at(extension).declareHandle(handle);
                    } else {
                        throw InvalidLoader("No loader found for " + extension + " file");
                    }
                } catch (const std::exception &e) {
                    throw e;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
