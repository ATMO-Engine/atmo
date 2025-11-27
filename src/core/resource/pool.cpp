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
                m_usedHandles = {};
                m_handles = {};
                m_resources = {};
                m_generations = {};
                m_freeList = {};
            }

            Pool::~Pool() {}

            const handle Pool::create(const std::string &path)
            {
                try {
                    if (m_handles.find(path) != m_handles.end()) {
                        return m_handles.at(path);
                    }

                    std::string extension = atmo::common::Utils::SplitString(path, '.').back();
                    ResourceFactory &fac = ResourceFactory::GetInstance();
                    std::shared_ptr<Resource> res = fac.create(extension);
                    res->load(path);

                    handle newHandle = {};
                    newHandle.frame_to_live = 1; // TODO: Définir un nombre de frame durant lequel la ressource peut vivre
                                                 // même sans être appelé
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

            std::shared_ptr<Resource> Pool::getFromHandle(const handle &handle)
            {
                if (handle.generation != m_generations.at(handle.index)) {
                    throw std::runtime_error("Handle périmé");
                }
                return m_resources.at(handle.index);
            }

            void Pool::declareHandle(const handle &handle)
            {
                auto it = std::find_if(m_usedHandles.begin(), m_usedHandles.end(), [&handle](const resource::handle &h) { return handle.path == h.path; });
                if (it != m_usedHandles.end()) {
                    m_usedHandles.push_back(handle);
                }
            }

            void Pool::destroy(const handle &handle)
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
