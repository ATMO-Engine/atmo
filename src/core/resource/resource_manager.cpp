#include "resource_manager.hpp"
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
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

            ResourceManager::ResourceManager() : m_factory(ResourceFactory::GetInstance())
            {
                m_pools = makePoolMap<LoaderTypes>();
            }

            ResourceManager &ResourceManager::GetInstance()
            {
                static ResourceManager instance;
                return instance;
            }


            const handle ResourceManager::generate(const std::string &path)
            {
                std::string extension = atmo::common::Utils::SplitString(path, '.').back();
                try {
                    if (m_pools.find(extension) != m_pools.end()) {
                        handle newHandle = m_pools.at(extension).create(path);
                        return newHandle;
                    } else {
                        throw std::runtime_error("No matching pool for the path given. Invalid file extension");
                    }
                } catch (const std::exception &e) {
                    std::cout << e.what() << std::endl;
                    throw e;
                }
            }

            std::shared_ptr<Resource> ResourceManager::getResource(const handle &handle)
            {
                std::string extension = atmo::common::Utils::SplitString(handle.path, '.').back();
                try {
                    if (m_pools.find(extension) != m_pools.end()) {
                        // create Resource class through a calss that return a Resource class thanks to the path
                        return m_pools.at(extension).getFromHandle(handle);
                    } else {
                        throw std::runtime_error("No matching pool for the handle given. Invalid file extension");
                    }
                } catch (const std::exception &e) {
                    std::cout << e.what() << std::endl;
                    throw e;
                }
            }

            void ResourceManager::declareHandle(const handle &handle)
            {
                std::string extension = atmo::common::Utils::SplitString(handle.path, '.').back();
                try {
                    if (m_pools.find(extension) != m_pools.end()) {
                        // create Resource class through a class that return a Resource class thanks to the path
                        m_pools.at(extension).declareHandle(handle);
                    } else {
                        throw std::runtime_error("No matching pool for the handle given. Invalid file extension");
                    }
                } catch (const std::exception &e) {
                    std::cout << e.what() << std::endl;
                    throw e;
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
