#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "core/types.hpp"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class SubResource
            {
            public:
                virtual ~SubResource() = default;

                virtual std::string_view name() const noexcept = 0;

                virtual std::string serialize() const = 0;
                virtual void deserialize(const std::string &data) = 0;
            };

            class SubResourceRegistry
            {
            public:
                using Factory = std::unique_ptr<SubResource> (*)();

                struct Entry {
                    bool is_abstract;
                    std::optional<Factory> factory;
                };

                template <typename T> static void RegisterType(std::string_view name)
                {
                    if constexpr (std::is_abstract_v<T>) {
                        Instance().m_registry[std::string(name)] = Entry{ .is_abstract = true, .factory = std::nullopt };
                    } else {
                        Instance().m_registry[std::string(name)] =
                            Entry{ .is_abstract = false, .factory = []() -> std::unique_ptr<SubResource> { return std::make_unique<T>(); } };
                    }
                }

                static std::unique_ptr<SubResource> Create(std::string_view name)
                {
                    auto it = Instance().m_registry.find(std::string(name));
                    if (it == Instance().m_registry.end()) {
                        spdlog::error("SubResource type '{}' not found in registry", name);
                        return nullptr;
                    }

                    if (it->second.is_abstract) [[unlikely]] {
                        spdlog::error("SubResource type '{}' is abstract and cannot be instantiated", name);
                        return nullptr;
                    }

                    return it->second.factory.value()();
                }

            private:
                std::unordered_map<std::string, Entry> m_registry;

                static SubResourceRegistry &Instance()
                {
                    static SubResourceRegistry registry;
                    return registry;
                }
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
