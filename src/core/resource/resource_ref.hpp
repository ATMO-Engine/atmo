#pragma once

#include <cstdint>
#include <memory>
#include "handle.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            template <typename T> class ResourcePool;

            template <typename T> class ResourceRef
            {
            public:
                ResourceRef() = default;

                ResourceRef(ResourcePool<T> *pool, StoreHandle handle, uint64_t tick) : m_pool(pool), m_handle(handle)
                {
                    m_tick = tick;
                    pin();
                }

                ResourceRef(const ResourceRef &other, uint64_t tick) : m_pool(other.m_pool), m_handle(other.m_handle)
                {
                    m_tick = tick;
                    pin();
                }

                ResourceRef(ResourceRef &&other) noexcept : m_pool(other.m_pool), m_handle(other.m_handle)
                {
                    other.m_pool = nullptr;
                }

                ~ResourceRef()
                {
                    unpin();
                }

                std::shared_ptr<T> get(uint64_t tick = -1)
                {
                    if (tick != -1) {
                        m_tick = tick;
                    }

                    if (m_pool) {
                        try {
                            return m_pool->getAsset(m_handle);
                        } catch (const typename ResourcePool<T>::HandleOutDated &e) {
                            return nullptr;
                        }
                    } else {
                        return nullptr;
                    }
                }

                T *operator->() const
                {
                    return get();
                }
                T &operator*() const
                {
                    return *get();
                }

            private:
                ResourcePool<T> *m_pool = nullptr;
                StoreHandle m_handle{};

                uint64_t m_tick = 0;

                /**
                 * @brief Pin the resource so it cannot be destroyed until unpin is called.
                 * This method has to be called to make sure the resource stay alive as long as the entity is alive.
                 * This method increase a counter of pinned ressources so for each pin() called unpin() has to be called whenever the resource is no longer used
                 * by the entity
                 */
                void pin()
                {
                    if (m_pool) {
                        m_pool->pin(m_handle, m_tick);
                    }
                }

                /**
                 * @brief Unpin the resource so it can be freed later by the resource manager.
                 * This method has to be called whenever an entity is destroyed or doesn't use the resource anymore.
                 */
                void unpin()
                {
                    if (m_pool) {
                        m_pool->unpin(m_handle, m_tick);
                    }
                }
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
