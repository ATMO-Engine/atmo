#include <exception>
#include <iostream>
#include <memory>
#include "common/utils.hpp"
#include "core/resource/loaders/script_loader.hpp"
#include "core/resource/resource_factory.hpp"
#include "loaders/image_loader.hpp"

#include "resource_manager.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ResourceManager::ResourceManager() : _factory(ResourceFactory::getInstance())
            {
                _pools = {
                    {"png", Pool()},
                    {"luau", Pool()}
                };

                _factory.registerLoader("png", []() {return std::make_shared<ImageLoader>(ImageLoader());});
                _factory.registerLoader("luau", []() {return std::make_shared<ScriptLoader>(ScriptLoader());});
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
                        throw std::exception("Invalid file extension");
                    }
                } catch (const std::exception &e) {
                    std::cout << e.what() << std::endl;
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
                        throw std::exception("No matching pool for the handle given. Invalid file extension");
                    }
                } catch (const std::exception &e) {
                    std::cout << e.what() << std::endl;
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
                        throw std::exception("No matching pool for the handle given. Invalid file extension");
                    }
                } catch (const std::exception &e) {
                    std::cout << e.what() << std::endl;
                    throw e;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
