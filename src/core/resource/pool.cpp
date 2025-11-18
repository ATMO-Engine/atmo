#include "core/resource/pool.hpp"
#include <algorithm>
#include <basetsd.h>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_set>
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
                m_usedHandles = {};
                m_handles = {};
                m_resources = {};
                m_generations = {};
                m_freeList = {};

                m_resourceMutex = std::make_unique<std::mutex>();
                m_handleManagementMutex = std::make_unique<std::mutex>();
            }

            Pool::~Pool() {}

            const Handle Pool::create(const std::string &path)
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

                    Handle newHandle = {};
                    newHandle.path = path;

                    if (!m_freeList.empty()) {
                        std::uint16_t idx = m_freeList.back();
                        m_freeList.pop_back();
                        m_resources.at(idx) = res;
                        newHandle.generation = m_generations.at(idx);
                        newHandle.index = idx;
                    } else {
                        newHandle.index = m_resources.size();
                        newHandle.generation = 1;
                        m_resources.push_back(res);
                        m_generations.push_back(newHandle.generation);
                    }

                    m_handles.insert(std::make_pair(path, newHandle));

                    return newHandle;
                } catch (const std::exception &e) {
                    throw e;
                }
            }

            std::shared_ptr<Resource> Pool::getFromHandle(const Handle &handle)
            {
                std::scoped_lock lock(*m_resourceMutex);
                if (handle.generation != m_generations.at(handle.index)) {
                    throw std::runtime_error("Handle périmé");
                }
                return m_resources.at(handle.index);
            }

            void Pool::declareHandle(const Handle &handle)
            {
                std::scoped_lock lock(*m_handleManagementMutex);
                auto path = handle.path;
                auto it = std::find_if(m_usedHandles.begin(), m_usedHandles.end(), [&path](const std::string &h) {
                    return path == h;
                });
                if (it != m_usedHandles.end()) {
                    m_usedHandles.push_back(handle.path);
                }
            }

            void Pool::clear()
            {
                std::scoped_lock lock(*m_handleManagementMutex, *m_resourceMutex);

                std::unordered_set<std::string> toKeep(m_usedHandles.begin(), m_usedHandles.end());
                for (auto it = m_handles.begin(); it != m_handles.end();) {
                    if (toKeep.find(it->first) == toKeep.end()) {
                        destroy(it->second);
                        it = m_handles.erase(it);
                    } else {
                        it++;
                    }
                }
                m_usedHandles.clear();
            }

            void Pool::clearHandle(const Handle &handle)
            {
                std::scoped_lock lock(*m_handleManagementMutex, *m_resourceMutex);

                auto itMap = m_handles.find(handle.path);
                if (itMap != m_handles.end()) {
                    m_handles.erase(itMap);
                }

                auto itVector = std::find(m_usedHandles.begin(), m_usedHandles.end(), handle.path);
                if (itVector != m_usedHandles.end()) {
                    m_usedHandles.erase(itVector);
                }
                destroy(handle);
            }

            void Pool::destroy(const Handle &handle)
            {
                if (handle.generation != m_generations.at(handle.index)) {
                    throw std::runtime_error("Handle périmé");
                }
                m_resources.at(handle.index)->destroy(); // TODO: Implementer avec le système de caching (retirer la
                                                        // ressource du vecteur et l'envoyer dans le cache)
                m_generations.at(handle.index) += 1;
                m_freeList.push_back(handle.index);
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
// namespace atmo
