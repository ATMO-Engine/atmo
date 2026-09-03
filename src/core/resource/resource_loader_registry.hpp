#pragma once

#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "core/resource/resource.hpp"

namespace atmo::core::resource
{
    namespace detail
    {
        template <typename T> struct TypeTag {
            static const char tag;
        };
        template <typename T> const char TypeTag<T>::tag = 0;
    } // namespace detail

    template <typename T> const void *ResourceTypeKey()
    {
        return &detail::TypeTag<T>::tag;
    }

    struct ResourceLoaderMetadata {
        std::string_view display_name;
        std::initializer_list<std::string_view> extensions;
    };

    class ResourceLoaderRegistry
    {
    public:
        static ResourceLoaderRegistry &Instance()
        {
            static ResourceLoaderRegistry instance;
            return instance;
        }

        ResourceLoaderRegistry(const ResourceLoaderRegistry &) = delete;
        ResourceLoaderRegistry &operator=(const ResourceLoaderRegistry &) = delete;

        template <typename T, typename LoaderClass> void registerLoader(ResourceLoaderMetadata metadata = {})
        {
            Entry entry;
            entry.metadata = metadata;
            entry.instance = { nullptr, [](void *) {} };
            entry.create = []() -> void * { return static_cast<Resource<T> *>(new LoaderClass()); };
            entry.destroy = [](void *ptr) { delete static_cast<Resource<T> *>(ptr); };

            m_entries[ResourceTypeKey<T>()] = std::move(entry);
        }

        template <typename T> Resource<T> *getOrCreateLoader()
        {
            auto it = m_entries.find(ResourceTypeKey<T>());
            if (it == m_entries.end()) {
                throw std::runtime_error("No resource loader registered for this type");
            }

            Entry &entry = it->second;
            if (!entry.instance) {
                entry.instance = std::unique_ptr<void, void (*)(void *)>(entry.create(), entry.destroy);
            }
            return static_cast<Resource<T> *>(entry.instance.get());
        }

        [[nodiscard]] const ResourceLoaderMetadata *findMetadata(const void *type) const
        {
            auto it = m_entries.find(type);
            return it != m_entries.end() ? &it->second.metadata : nullptr;
        }

    private:
        ResourceLoaderRegistry() = default;

        struct Entry {
            ResourceLoaderMetadata metadata;
            std::unique_ptr<void, void (*)(void *)> instance{ nullptr, [](void *) {} };
            void *(*create)() = nullptr;
            void (*destroy)(void *) = nullptr;
        };

        std::unordered_map<const void *, Entry> m_entries;
    };
} // namespace atmo::core::resource
