#include "core/resource/resource_pool.hpp"
#include <basetsd.h>
#include "core/resource/resource_pool.hpp"
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include "common/utils.hpp"
#include "core/resource/resource.hpp"
#include "core/resource/resource_factory.hpp"
#include "handle.hpp"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ResourcePool::ResourcePool()
            {
                m_handles = {};
                m_resources = {};
                m_generations = {};
                m_freeList = {};

                m_resourceMutex = std::make_unique<std::mutex>();
                m_handleManagementMutex = std::make_unique<std::mutex>();
            }

            ResourcePool::~ResourcePool() {}

            const Handle ResourcePool::create(const std::string &path)
            {
                try {
                    std::scoped_lock lock(*m_resourceMutex);

                    if (m_handles.find(path) != m_handles.end()) {
                        return m_handles.at(path);
                    }

                    std::string extension = atmo::common::Utils::splitString(path, '.').back();
                    ResourceFactory &fac = ResourceFactory::getInstance();
                    std::shared_ptr<Resource> res = fac.create(extension);
                    res->load(path);

                    Handle newHandle = std::make_shared<__Handle>();
                    newHandle->path = path;

                    if (!m_freeList.empty()) {
                        std::uint16_t idx = m_freeList.back();
                        m_freeList.pop_back();
                        m_resources.at(idx) = res;
                        newHandle->generation = m_generations.at(idx);
                        newHandle->index = idx;
                    } else {
                        newHandle->index = m_resources.size();
                        newHandle->generation = 1;
                        m_resources.push_back(res);
                        m_generations.push_back(newHandle->generation);
                    }

                    m_handles.insert(std::make_pair(path, newHandle));

                    return newHandle;
                } catch (const std::exception &e) {
                    throw e;
                }
            }

            std::shared_ptr<Resource> ResourcePool::getFromHandle(const Handle &handle)
            {
                std::scoped_lock lock(*m_resourceMutex);
                if (handle->generation != m_generations.at(handle->index)) {
                    throw std::runtime_error("Handle périmé");
                }
                return m_resources.at(handle->index);
            }

            void ResourcePool::clear()
            {
                for (auto it = m_handles.begin(); it != m_handles.end();) {
                    if (it->second.use_count() == 1) {
                        spdlog::info("\tclear: " + it->first);
                        destroy(it->second);
                        it = m_handles.erase(it);
                    } else {
                        it++;
                    }
                }
            }

            void ResourcePool::destroy(const Handle &handle)
            {
                if (handle->generation != m_generations.at(handle->index)) {
                    throw std::runtime_error("Handle périmé");
                }
                m_resources.at(handle->index)->destroy(); // TODO: Implementer avec le système de caching (retirer la
                                                        // ressource du vecteur et l'envoyer dans le cache)
                m_generations.at(handle->index) += 1;
                m_freeList.push_back(handle->index);
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
