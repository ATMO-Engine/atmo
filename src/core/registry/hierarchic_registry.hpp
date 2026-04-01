#pragma once

#include <concepts>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "spdlog/spdlog.h"

namespace atmo::core::registry
{
    template <typename Registry, typename Root, typename... FactoryArgs> class HierarchicRegistry
    {
    public:
        template <typename Type> static void RegisterType()
        {
            if (Instance().p_registry.contains(Type::FullName().data())) {
                spdlog::error(R"("{}" is already registered in registry)", Type::FullName());
                return;
            }

            if constexpr (std::is_abstract_v<Type>) {
                Instance().p_registry[std::string(Type::FullName())] = Entry{ .is_abstract = true, .factory = std::nullopt };
                spdlog::debug(R"(Registered abstract type "{}")", Type::FullName());
            } else {
                Instance().p_registry[std::string(Type::FullName())] = Entry{ .is_abstract = false, .factory = Registry::template Factorize<Type> };
                spdlog::debug(R"(Registered type "{}")", Type::FullName());
            }

            Registry::template OnRegister<Type>();
        }

        template <class Derived, class Base> struct Registrable : Base {
            using Base::Base;

            static constexpr std::string_view FullName()
            {
                static const std::string full_name = std::format("{}::{}", Base::FullName(), Derived::LocalName());
                return full_name;
            }
        };

        // Method used to use "Instance().p_registry | std::views::keys | std::ranges::to<std::vector>()" but it isn't supported on enough compilers yet
        static std::vector<std::string> GetEntries()
        {
            std::vector<std::string> entries;
            for (const auto &[key, _] : Instance().p_registry) {
                entries.push_back(key);
            }
            return entries;
        }

        template <typename T = Root>
            requires std::derived_from<T, Root>
        static std::shared_ptr<T> Create(std::string_view name, FactoryArgs... args)
        {
            auto &registry = Instance().p_registry;

            auto it = registry.find(std::string(name));
            if (it == registry.end()) [[unlikely]] {
                spdlog::error(R"("{}" not found in registry)", name);
                return nullptr;
            }

            if (it->second.is_abstract) [[unlikely]] {
                spdlog::error(R"("{}" is abstract and cannot be instantiated)", name);
                return nullptr;
            }

            Root *basePtr = it->second.factory.value()(args...);

            return std::shared_ptr<T>(static_cast<T *>(basePtr));
        }

        template <typename Type> static void OnRegister() {};
        template <typename Type> static Root *Factorize();

        HierarchicRegistry(const HierarchicRegistry &) = delete;
        HierarchicRegistry &operator=(const HierarchicRegistry &) = delete;

        static Registry &Instance()
        {
            static Registry registry;
            return registry;
        }

    protected:
        HierarchicRegistry() = default;
        ~HierarchicRegistry() = default;

        using Factory = Root *(*)(FactoryArgs...);

        struct Entry {
            bool is_abstract;
            std::optional<Factory> factory;
        };

        std::unordered_map<std::string, Entry> p_registry;
    };
} // namespace atmo::core::registry
