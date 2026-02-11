#pragma once

#include <memory>
#include <string_view>
#include <type_traits>

#include "spdlog/spdlog.h"

namespace atmo::core::registry
{
    template <typename Registry, typename Root> class HierarchicRegistry
    {
    public:
        template <typename T> static void RegisterType()
        {
            std::string name;

            if constexpr (std::is_same_v<T, Root>) {
                name = std::string(name);
            } else {
                name = std::string(typeid(T).name());
            }

            if constexpr (std::is_abstract_v<T>) {
                Instance().m_registry[std::string(name)] = Entry{ .is_abstract = true, .factory = std::nullopt };
                spdlog::info(R"(Registered abstract type "{}" in registry "{}")", name, typeid(Registry).name());
            } else {
                Instance().m_registry[std::string(name)] =
                    Entry{ .is_abstract = false, .factory = []() -> std::unique_ptr<Root> { return std::make_unique<T>(); } };
                spdlog::info(R"(Registered type "{}" in registry "{}")", name, typeid(Registry).name());
            }
        }

        static std::unique_ptr<Root> Create(std::string_view name)
        {
            auto it = Instance().m_registry.find(std::string(name));
            if (it == Instance().m_registry.end()) {
                spdlog::error(R"("{}" not found in registry)", name);
                return nullptr;
            }

            if (it->second.is_abstract) [[unlikely]] {
                spdlog::error(R"("{}" is abstract and cannot be instantiated)", name);
                return nullptr;
            }

            return it->second.factory.value()();
        }

        HierarchicRegistry(const HierarchicRegistry &) = delete;
        HierarchicRegistry &operator=(const HierarchicRegistry &) = delete;

    protected:
        HierarchicRegistry() = default;
        ~HierarchicRegistry() = default;

        static Registry &Instance()
        {
            static Registry registry;
            return registry;
        }

    private:
        using Factory = std::unique_ptr<Root> (*)();

        struct Entry {
            bool is_abstract;
            std::optional<Factory> factory;
        };

        std::unordered_map<std::string, Entry> m_registry;
    };
} // namespace atmo::core::registry
