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
            template<typename T>
            class ResourcePool;

            template<typename T>
            class ResourceRef
            {
                public:
                    ResourceRef() = default;

                    ResourceRef(ResourcePool<T> *pool, StoreHandle handle, uint64_t tick) : m_pool(pool), m_handle(handle)
                    {
                        retain(tick);
                    }

                    ResourceRef(const ResourceRef &other, uint64_t tick) : m_pool(other.m_pool), m_handle(other.m_handle)
                    {
                        retain(tick);
                    }

                    ResourceRef(ResourceRef &&other) noexcept : m_pool(other.m_pool), m_handle(other.m_handle)
                    {
                        other.m_pool = nullptr;
                    }

                    ~ResourceRef()
                    {
                        release();
                    }

                    std::shared_ptr<T> get() const
                    {
                        if (m_pool) {
                            return m_pool->getAsset(m_handle);
                        } else {
                            return nullptr;
                        }
                    }

                    T *operator->() const { return get(); }
                    T &operator*() const { return *get(); }
                private:
                    ResourcePool<T> *m_pool = nullptr;
                    StoreHandle m_handle{};

                    void retain(uint64_t tick)
                    {
                        if (m_pool) {
                            m_pool->retain(m_handle, tick);
                        }
                    }

                    void release() {
                        if (m_pool) {
                            m_pool->release(m_handle);
                        }
                    }
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
