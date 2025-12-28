#pragma once

#include "handle.hpp"
#include "resource_pool.hpp"
namespace atmo
{
    namespace core
    {
        namespace resource
        {
            template<typename T>
            class ResourceRef
            {
                public:
                    ResourceRef() = default;

                    ResourceRef(ResourcePool<T> *pool, Handle<T> handle) : m_pool(pool), m_handle(handle)
                    {
                        retain();
                    }

                    ResourceRef(const ResourceRef &other) : m_pool(other.m_pool), m_handle(other.m_handle)
                    {
                        retain();
                    }

                    ResourceRef(ResourceRef &&other) noexcept : m_pool(other.m_pool), m_handle(other.m_handle)
                    {
                        other.m_pool = nullptr;
                    }

                    ResourceRef& operator=(const ResourceRef& rhs)
                    {
                        if (this != &rhs)
                        {
                            release();
                            m_pool = rhs.m_pool;
                            m_handle = rhs.m_handle;
                            retain();
                        }
                        return *this;
                    }

                    ~ResourceRef()
                    {
                        release();
                    }

                    T *get() const
                    {
                        if (m_pool) {
                            m_pool->get(m_handle);
                        } else {
                            return nullptr;
                        }
                    }

                    T *operator->() const { return get(); }
                    T &operator*() const { return *get(); }

                private:
                    ResourcePool<T> *m_pool = nullptr;
                    Handle<T> m_handle{};

                    void retain()
                    {
                        if (m_pool) {
                            m_pool->retain();
                        }
                    }

                    void release() {
                        if (m_pool) {
                            m_pool->release();
                        }
                    }
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
