#include "resource_manager.hpp"
#include <exception>
#include <memory>
#include <stdexcept>
#include "common/utils.hpp"
#include "core/resource/resource_factory.hpp"
#include "spdlog/spdlog.h"

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

            ResourceManager::ResourceManager() : m_factory(ResourceFactory::getInstance())
            {
                m_pools = makePoolMap<LoaderTypes>();
            }

            ResourceManager &ResourceManager::GetInstance()
            {
                static ResourceManager instance;
                return instance;
            }


            const Handle ResourceManager::generate(const std::string &path)
            {
                std::string extension = atmo::common::Utils::splitString(path, '.').back();
                try {
                    if (m_pools.find(extension) != m_pools.end()) {
                        Handle newHandle = m_pools.at(extension).create(path);
                        return newHandle;
                    } else {
                        throw std::runtime_error("No matching pool for the path given. Invalid file extension");
                    }
                } catch (const std::exception &e) {
                    spdlog::error(e.what());
                    throw e;
                }
            }

            std::shared_ptr<Resource> ResourceManager::getResource(const Handle &handle)
            {
                std::string extension = atmo::common::Utils::splitString(handle->path, '.').back();
                try {
                    if (m_pools.find(extension) != m_pools.end()) {
                        // create Resource class through a calss that return a Resource class thanks to the path
                        return m_pools.at(extension).getFromHandle(handle);
                    } else {
                        throw std::runtime_error("No matching pool for the handle given. Invalid file extension");
                    }
                } catch (const std::exception &e) {
                    spdlog::error(e.what());
                    throw e;
                }
            }

            void ResourceManager::clear()
            {
                spdlog::info("Clear handles started");
                for (auto &pool : m_pools) {
                    spdlog::info(pool.first + " clear start:");
                    pool.second.clear();
                    spdlog::info(pool.first + " clear ended");
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
