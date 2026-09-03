#pragma once

#include <concepts>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/types.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::registry
{
    template <typename Registry, typename Root, typename... FactoryArgs> class HierarchicRegistry
    {
    public:
        template <typename Type> static void RegisterType(std::string_view icon_path = "", std::optional<types::Color> icon_color = std::nullopt)
        {
            if (Instance().p_registry.contains(Type::FullName().data())) {
                spdlog::error(R"("{}" is already registered in registry)", Type::FullName());
                return;
            }

            Entry entry{ .icon = icon_path, .icon_color = icon_color };

            // if (icon_path.empty()) {
            //     entry.icon = Instance().p_registry[std::string(Type::BaseFullName())].icon;
            // } else {
            //     entry.icon = icon_path;
            // }

            // if (!icon_color.has_value()) {
            //     entry.icon_color = Instance().p_registry[std::string(Type::BaseFullName())].icon_color;
            // } else {
            //     entry.icon_color = icon_color;
            // }

            if constexpr (std::is_abstract_v<Type>) {
                entry.is_abstract = true;
                entry.factory = std::nullopt;
                Instance().p_registry[std::string(Type::FullName())] = entry;
                spdlog::debug(R"(Registered abstract type "{}")", Type::FullName());
            } else {
                entry.is_abstract = false;
                entry.factory = Registry::template Factorize<Type>;
                Instance().p_registry[std::string(Type::FullName())] = entry;
                spdlog::debug(R"(Registered type "{}")", Type::FullName());
            }

            Registry::template OnRegister<Type>();
        }

        template <class Derived, class Base> struct Registrable : Base {
            using Base::Base;

            static constexpr std::string_view BaseFullName()
            {
                return Base::FullName();
            }

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

        struct EntryTree {
            std::string name = "";
            std::vector<EntryTree> children = {};
        };

        static void MakeTree(std::vector<std::string> *entries, EntryTree *parent)
        {
            while (!entries->empty()) {
                auto entity_name = entries->front();

                if (!entity_name.starts_with(parent->name))
                    return;

                EntryTree tree;
                tree.name = entity_name;

                entries->erase(entries->begin());

                MakeTree(entries, &tree);

                parent->children.emplace_back(tree);
            }
        }

        static EntryTree GetEntriesTree()
        {
            std::vector<std::string> entries = GetEntries();
            EntryTree entries_tree;

            auto compare = [](std::string a, std::string b) { return a < b; };
            std::sort(entries.begin(), entries.end(), compare);
            entries_tree.name = entries[0];
            entries.erase(entries.begin());
            MakeTree(&entries, &entries_tree);
            return entries_tree;
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

        static bool IsAbstract(std::string_view name)
        {
            auto &registry = Instance().p_registry;
            auto it = registry.find(std::string(name));
            return it->second.is_abstract;
        }

        static std::string GetIconPath(std::string_view name)
        {
            std::string icon_path = std::string(Instance().p_registry[std::string(name)].icon);

            if (icon_path.empty())
                icon_path = GetIconPath(name.substr(0, name.find_last_of("::")));

            return icon_path;
        }

        static types::Color GetIconColor(std::string_view name)
        {
            std::optional<types::Color> icon_color = Instance().p_registry[std::string(name)].icon_color;

            if (!icon_color.has_value())
                icon_color = GetIconColor(name.substr(0, name.find_last_of("::")));

            return icon_color.value();
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
            bool is_abstract = false;
            std::optional<Factory> factory;

            std::string_view icon = "";
            std::optional<types::Color> icon_color = std::nullopt;
        };

        std::unordered_map<std::string, Entry> p_registry;
    };
} // namespace atmo::core::registry
