#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "flecs.h"

#include "meta/component_meta.hpp"
#include "meta/type_info.hpp"

namespace atmo::meta
{
    template <HasComponentMeta T> void register_flecs_meta(flecs::world &world);
}

namespace atmo::meta
{
    class MetaRegistry
    {
    public:
        static MetaRegistry &Instance()
        {
            static MetaRegistry instance;
            return instance;
        }

        MetaRegistry(const MetaRegistry &) = delete;
        MetaRegistry &operator=(const MetaRegistry &) = delete;

        /**
         * @brief Register a type T with the MetaRegistry.
         *
         * @tparam T Type to register. Must have a ComponentMeta<T> struct defined.
         */
        template <HasComponentMeta T> void registerType()
        {
            const std::string_view type_name = ComponentMeta<T>::name;
            for (const auto &ti : m_types) {
                if (std::string_view{ ti.name } == type_name) {
                    return;
                }
            }

            TypeInfo ti = make_type_info<T>();
            ti.register_flecs = &register_flecs_meta<T>;
            m_types.push_back(std::move(ti));
        }

        /**
         * @brief Register all types in the registry with the flecs meta addon.
         *
         * @param world flecs world to register types with.
         */
        void registerAllFlecs(flecs::world &world)
        {
            for (auto &ti : m_types) {
                if (ti.register_flecs) {
                    ti.register_flecs(world);
                    if (ti.resolve_flecs_id) {
                        ti.flecs_id = ti.resolve_flecs_id(world);
                    }
                }
            }
        }

        /**
         * @brief Find a registered type by name.
         *
         * @param type_name Name of the type to find.
         * @return const TypeInfo* Pointer to the TypeInfo of the found type, or nullptr if not found.
         */
        [[nodiscard]] const TypeInfo *find(std::string_view type_name) const
        {
            for (const auto &ti : m_types) {
                if (std::string_view{ ti.name } == type_name) {
                    return &ti;
                }
            }
            return nullptr;
        }

        [[nodiscard]] const TypeInfo *findByFlecsId(std::uint64_t id) const
        {
            for (const auto &ti : m_types) {
                if (ti.flecs_id == id) {
                    return &ti;
                }
            }
            return nullptr;
        }

        template <HasComponentMeta T> [[nodiscard]] const TypeInfo *lookup() const
        {
            return find(ComponentMeta<T>::name);
        }

        /**
         * @brief Get a span of all registered types in the registry.
         *
         * @return std::span<const TypeInfo>
         */
        [[nodiscard]] std::span<const TypeInfo> all() const
        {
            return m_types;
        }

    private:
        MetaRegistry() = default;

        std::vector<TypeInfo> m_types;
    };

} // namespace atmo::meta
