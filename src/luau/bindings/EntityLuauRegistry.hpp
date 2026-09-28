#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <flecs.h>

#include "core/ecs/entities/entity.hpp"
#include "spdlog/spdlog.h"

#define ATMO_REGISTER_LUA_ENTITY_BINDING(entityType)                                                     \
namespace                                                                                                \
{                                                                                                        \
    static int _ = [] {                                                                                  \
        using Binding = atmo::luau::LuaBindings<entityType>;                                             \
        atmo::luau::LuauRegistry::Instance().registerType(                                               \
            Binding::name,                                                                               \
            atmo::luau::LuaEntry{                                                                        \
                Binding::name,                                                                           \
                Binding::ParentName(),                                                                   \
                [](lua_State *L) { Binding::RegisterType(L); },                                          \
                [](lua_State *L, flecs::entity &e) { Binding::Push(L, entityType(e)); }                  \
            });                                                                                          \
        return 0;                                                                                        \
    }();                                                                                                 \
}

#define ATMO_REGISTER_LUA_COMPONENT_BINDING(entityType)                                                  \
namespace                                                                                                \
{                                                                                                        \
    static int _ = [] {                                                                                  \
        using Binding = atmo::luau::LuaBindings<entityType>;                                             \
        atmo::luau::LuauRegistry::Instance().registerType(                                               \
            Binding::name,                                                                               \
            atmo::luau::LuaEntry{                                                                        \
                Binding::name,                                                                           \
                Binding::ParentName(),                                                                   \
                [](lua_State *L) { Binding::RegisterType(L); },                                          \
                nullptr                                                                                  \
            });                                                                                          \
        return 0;                                                                                        \
    }();                                                                                                 \
}

namespace atmo::luau {
    struct LuaEntry
    {
        const char *metatable_name;
        const char *parent_name;
        std::function<void(lua_State *)> register_type;
        std::function<void(lua_State *, flecs::entity &)> push_entity;
    };

    class LuauRegistry
    {
    public:
        static LuauRegistry &Instance()
        {
            static LuauRegistry instance;
            return instance;
        }

        void registerType(const char *name, LuaEntry entry)
        {
            m_entries[name] = std::move(entry);
        }

        const LuaEntry *find(const char *name) const
        {
            auto it = m_entries.find(name);
            return it != m_entries.end() ? &it->second : nullptr;
        }

        /**
         * @brief
         * Push the entity with the binding of its type. If the type has no binding, the closest
         * ancestor that has one is used (type_name is the full hierarchy, e.g. "Entity::Entity2d").
         *
         * @param L The vm in which the entity is pushed
         * @param e The entity to push
         * @return true if the entity has been pushed, false if no binding matches (nothing pushed)
         */
        bool pushEntity(lua_State *L, flecs::entity &e) const
        {
            const auto *base = e.try_get<core::components::EntityBase>();
            if (!base) {
                spdlog::error("LuauRegistry: entity '{}' has no EntityBase component", e.name().c_str());
                return false;
            }

            std::string_view typeName = base->type_name;
            while (!typeName.empty()) {
                const size_t sep = typeName.rfind("::");
                const std::string localName(sep == std::string_view::npos ? typeName : typeName.substr(sep + 2));

                const LuaEntry *entry = find(localName.c_str());
                if (entry && entry->push_entity) {
                    entry->push_entity(L, e);
                    return true;
                }
                typeName = sep == std::string_view::npos ? std::string_view{} : typeName.substr(0, sep);
            }

            spdlog::error("LuauRegistry: no Lua binding registered for entity type '{}'", base->type_name);
            return false;
        }

        /**
         * @brief
         * Call the RegisterType method of each bindings that call
         * ATMO_REGISTER_LUA_ENTITY_BINDING or ATMO_REGISTER_LUA_COMPONENT_BINDING
         * @param L The vm in which the entity is pushed
         */
        void registerAll(lua_State *L)
        {
            std::unordered_map<std::string, bool> done;
            for (auto &[name, entry] : m_entries)
                registerOne(L, name, done);
        }

    private:
        LuauRegistry() = default;

        void registerOne(lua_State *L, const std::string &name, std::unordered_map<std::string, bool> &done)
        {
            if (done[name])
                return;

            auto it = m_entries.find(name);
            if (it == m_entries.end()) {
                spdlog::error("LuauRegistry: '{}' has no binding registered", name);
                return;
            }

            auto &entry = it->second;
            if (entry.parent_name)
                registerOne(L, entry.parent_name, done);

            entry.register_type(L);
            done[name] = true;
        }

        std::unordered_map<std::string, LuaEntry> m_entries;
    };
} // namespace atmo::luau
