#pragma once

#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "spdlog/spdlog.h"

namespace atmo::core::registry
{
    template <typename Registry, typename Root> class HierarchicRegistry
    {
    public:
        template <typename Type> static void RegisterType()
        {
            if constexpr (std::is_abstract_v<Type>) {
                Instance().m_registry[std::string(Type::FullName())] = Entry{ .is_abstract = true, .factory = std::nullopt };
                spdlog::debug(R"(Registered abstract type "{}")", Type::FullName());
            } else {
                Instance().m_registry[std::string(Type::FullName())] = Entry{ .is_abstract = false, .factory = Registry::template Factorize<Type> };
                spdlog::debug(R"(Registered type "{}")", Type::FullName());
            }
        }

        template <class Derived, class Base> struct Registrable : Base {
            using Base::Base;

            static constexpr std::string_view FullName()
            {
                static const std::string full_name = std::format("{}::{}", Base::FullName(), Derived::LocalName());
                return full_name;
            }
        };

        static std::vector<std::string> GetEntries()
        {
            return Instance().m_registry | std::views::keys | std::ranges::to<std::vector>();
        }

        static std::unique_ptr<Root> Create(std::string_view name)
        {
            auto it = Instance().m_registry.find(std::string(name));
            if (it == Instance().m_registry.end()) [[unlikely]] {
                spdlog::error(R"("{}" not found in registry)", name);
                return nullptr;
            }

            if (it->second.is_abstract) [[unlikely]] {
                spdlog::error(R"("{}" is abstract and cannot be instantiated)", name);
                return nullptr;
            }

            return it->second.factory.value()();
        }

        template <typename Type> static std::unique_ptr<Root> Factorize();

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
