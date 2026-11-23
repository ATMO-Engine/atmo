---
id: luau-bindings
title: Adding a Luau binding
order: 17
description: For engine contributors. Expose an entity, a component, a value or a set of functions to Luau scripts.
---

# Adding a Luau binding

This page is for contributors to the engine. A binding is a specialization of [[LuaBindings]] that describes a C++ type to
Luau. What scripts can use today is in [[luau-api]].

## Choose the base class {#base}

| To expose                       | Derive from                                 | Directory                     | What the script holds                                    |
| ------------------------------- | ------------------------------------------- | ----------------------------- | -------------------------------------------------------- |
| An entity type                  | [[EntityLuaBindings]]                       | `src/luau/bindings/entity`    | An entity handle                                         |
| A component                     | [[ComponentLuaBindings]]                    | `src/luau/bindings/component` | The entity. The component is found again at each access. |
| A small value, such as a vector | [[ValueLuaBindings]]                        | `src/luau/bindings/value`     | A copy                                                   |
| A shared resource               | [[ValueLuaBindings]] of a `std::shared_ptr` | `src/luau/bindings/resource`  | A shared pointer                                         |
| Functions, such as `Input`      | None                                        | `src/luau/bindings`           | A global table: see [functions](#functions)              |

Scripts never hold a pointer to the memory of a component, because Flecs moves it.

## An entity {#entity}

This exposes [[Camera2d::getZoom]] and [[Camera2d::setZoom]], which have no binding today.

`bindings_camera_entity.hpp`:

```cpp
#pragma once

#include "core/ecs/entities/2d/camera_2d/camera_2d.hpp"
#include "lualib.h"
#include "luau/bindings/entity/bindings_entity.hpp"
#include "luau/bindings/lua_bindings.hpp"

namespace atmo::luau
{
    template <>
    class LuaBindings<core::ecs::entities::Camera2d>
        : public EntityLuaBindings<LuaBindings<core::ecs::entities::Camera2d>, core::ecs::entities::Camera2d, LuaBindings<core::ecs::entities::Entity>>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, Index, "Camera2d.__index");
            lua_setfield(state, -2, "__index");
            lua_pushcfunction(state, NewIndex, "Camera2d.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);
            lua_pushcfunction(state, GetZoom, "Camera2d.getZoom");
            lua_setfield(state, -2, "getZoom");
            lua_pushcfunction(state, SetZoom, "Camera2d.setZoom");
            lua_setfield(state, -2, "setZoom");
            ChainTable(state, LuaBindings<core::ecs::entities::Entity>::name, "__methods");
            lua_setfield(state, -2, "__methods");

            RegisterProperties(state);
            RegisterIsA(state);
            Seal(state);
            lua_pop(state, 1);
        }

        static Property m_properties[];
        static constexpr const char *name = "Camera2d";

    private:
        static int GetZoom(lua_State *state);
        static int SetZoom(lua_State *state);
    };
} // namespace atmo::luau
```

`bindings_camera_entity.cpp`:

```cpp
#include "bindings_camera_entity.hpp"

#include "luau/bindings/EntityLuauRegistry.hpp"

namespace atmo::luau
{
    Property LuaBindings<core::ecs::entities::Camera2d>::m_properties[] = { { nullptr, nullptr, nullptr } };

    int LuaBindings<core::ecs::entities::Camera2d>::GetZoom(lua_State *state)
    {
        auto *camera = CheckPtr(state, 1);
        if (!camera->getHandle().is_alive()) {
            lua_pushnil(state);
            return 1;
        }
        lua_pushnumber(state, camera->getZoom());
        return 1;
    }

    int LuaBindings<core::ecs::entities::Camera2d>::SetZoom(lua_State *state)
    {
        auto *camera = CheckPtr(state, 1);
        const float zoom = static_cast<float>(luaL_checknumber(state, 2));
        if (camera->getHandle().is_alive())
            camera->setZoom(zoom);
        return 0;
    }
} // namespace atmo::luau

ATMO_REGISTER_LUA_ENTITY_BINDING(atmo::core::ecs::entities::Camera2d);
```

`RegisterType` builds the metatable of the type, in this order:

1. `__index` and `__newindex`, always [[LuaBindingsBase::Index]] and [[LuaBindingsBase::NewIndex]].
2. The `__methods` table. [[LuaBindingsBase::ChainTable]] adds the methods of the parent.
3. [[LuaBindingsBase::RegisterProperties]], even without properties.
4. [[LuaBindingsBase::RegisterIsA]], which lets the methods of `Entity` accept the type. Only for entities.
5. [[LuaBindingsBase::Seal]], last: scripts can then no longer change the metatable.

### How a binding is chosen {#lookup}

When an entity is given to a script, [[LuauRegistry::pushEntity]] reads its full name from the end, and the first type
with a binding wins. `Entity::Entity2d::Camera2d` looks for `Camera2d`, then `Entity2d`, then `Entity`. This is why `name`
must be the `LocalName` of the entity.

## Properties {#properties}

`m_properties` ends with `{ nullptr, nullptr, nullptr }`. Use these functions to fill it:

| Function                | For                                                                 |
| ----------------------- | ------------------------------------------------------------------- |
| [[makeFloatProperty]]   | A `float`                                                           |
| [[makeVector2Property]] | A vector: it is read as a copy, and written whole                   |
| [[makeProperty]]        | A number or a string                                                |
| [[makeValueProperty]]   | A field of a value type: reading works, and writing raises an error |

A property without a setter is read-only:

```cpp
Property LuaBindings<Transform2d>::m_properties[] = { makeVector2Property("position", &Transform2d::position),
                                                      makeFloatProperty("rotation", &Transform2d::rotation),
                                                      { nullptr, nullptr, nullptr } };
```

## Components and values {#components}

Follow `bindings_transform2` and `bindings_vector2`. They are registered with [[ATMO_REGISTER_LUA_COMPONENT_BINDING]],
which is also the macro for values and resources. Give them to scripts with:

| Function                                | Gives                                      |
| --------------------------------------- | ------------------------------------------ |
| [[ComponentLuaBindings::PushComponent]] | The component of an entity                 |
| [[ComponentLuaBindings::PushOwned]]     | An object made with `new`, deleted by Luau |
| [[ValueLuaBindings::Push]]              | A copy                                     |

For a constructor, such as `Vector2(1, 2)`, set a global function with the name of the type after `lua_pop`.

## Functions {#functions}

For a group of functions such as `Input`, write a class with a static `RegisterType` that fills a table and calls
`lua_setglobal`, as [[InputBindings]] does, then call it from [[Luau::registerBindings]]. It must be there: globals cannot
be added once `luaL_sandbox` has been called.

## Rules {#rules}

- Read the arguments with the `luaL_check` functions, which raise clear errors. Read bound types with
  [[LuaBindingsBase::CheckPtr]].
- For an entity, `CheckPtr` only checks the type. Check `is_alive()` before any access.
- Return `nil` for what is missing, and raise an error with `luaL_error` for a wrong use.
- `luaL_error` does not return. Do not call it while objects with a destructor, such as a `std::string`, are alive unless
  they are safe to unwind.
- Scripts run inside systems, while the world is read-only. Creating or destroying an entity, or adding a component, must
  go through [[SignalQueue::Enqueue]], as [[PrefabBindings]] does.
- A callback given by a script is kept with [[LuauRef]] and called with [[Luau::ProtectedCall]].
- Name methods and properties in `camelCase`, and types like the C++ ones.
- One registration macro per `.cpp` file.

## Check it {#check}

1. Try it with the `luau_eval` tool of the [[mcp-server]], or with a script.
2. Add checks to `assets/script/bindings_test.luau`, which logs `[PASS]` or `[FAIL]`.
3. Document it in [[luau-api]].
