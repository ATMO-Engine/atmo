---
id: adding-entities
title: Adding an entity or a component
order: 16
description: For engine contributors. Declare an entity type and its component, and register them with the engine.
---

# Adding an entity or a component

This page is for contributors to the engine. It adds `Coin2d`, an entity with a `Coin` component. The build compiles every
`.cpp` file of `src/`: there is no list of files to update.

## Header {#header}

```cpp
#pragma once

#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/ecs/entity_registry.hpp"
#include "meta/meta.hpp"

namespace atmo::core::components
{
    struct Coin {
        int value = 1;
        float spin_speed = 90.0f;
        bool collected = false;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Coin> {
    static constexpr const char *name = "Coin";
    static constexpr const char *category = "2D";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::Coin::value>("value"),
        atmo::meta::field<&atmo::core::components::Coin::spin_speed>("spin_speed").withRange(0.0f, 720.0f));
};

namespace atmo::core::ecs::entities
{
    class Coin2d : public EntityRegistry::Registrable<Coin2d, Entity2d>
    {
    public:
        using EntityRegistry::Registrable<Coin2d, Entity2d>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Coin2d";
        }

        void collect();
    };
} // namespace atmo::core::ecs::entities
```

## Source {#source}

```cpp
#include "coin_2d.hpp"

#include "core/ecs/world_context.hpp"
#include "meta/auto_register.hpp"

namespace atmo::core::ecs::entities
{
    void Coin2d::RegisterSystems(flecs::world *world)
    {
        const auto *ctx = world->try_get<components::WorldContext>();
        if (ctx && ctx->is_editor_isolated)
            return;

        world->system<components::Coin, components::Transform2d>("Coin2d_Spin")
            .kind(flecs::OnUpdate)
            .each([](flecs::entity e, components::Coin &coin, components::Transform2d &transform) {
                if (!coin.collected)
                    transform.rotation += coin.spin_speed * e.world().delta_time();
            });
    }

    void Coin2d::initialize()
    {
        Entity2d::initialize();

        setComponent<components::Coin>({});
        createSignal<int>("Collected");
    }

    void Coin2d::collect()
    {
        auto &coin = getComponentMutable<components::Coin>();
        if (coin.collected)
            return;
        coin.collected = true;
        getSignal<int>("Collected").emit(coin.value);
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Coin2d);
ATMO_REGISTER_COMPONENT(atmo::core::components::Coin)
```

## The entity {#entity}

`Registrable<Coin2d, Entity2d>` gives the entity its full name, made of those of its parents and its `LocalName`:
`Entity::Entity2d::Coin2d`. [[EntityRegistry]] creates entities by full name:

```cpp
auto coin = EntityRegistry::Create<entities::Coin2d>("Entity::Entity2d::Coin2d");
coin->setParent(parent);
```

- **`RegisterSystems` must be declared**, even when it is empty. Otherwise the one of the parent is found instead, and its
  systems are registered twice.
- **`initialize` must call the one of the parent first**, which adds the components of the parent.
- An abstract class, such as `Body2d`, is registered but cannot be created.
- An entity object is only a handle. Copying it does not copy the entity, and all its data must be in components.
- Use `CreateIn` to create an entity in the world of a scene editor. `Create` uses the main world.

### Register it {#register-entity}

| Macro                                                | Also sets              |
| ---------------------------------------------------- | ---------------------- |
| `ATMO_REGISTER_ENTITY(type)`                         | Nothing                |
| `ATMO_REGISTER_ENTITY_ICON(type, icon)`              | The icon in the editor |
| `ATMO_REGISTER_ENTITY_COLOR(type, color)`            | The color of the icon  |
| `ATMO_REGISTER_ENTITY_ICON_COLOR(type, icon, color)` | Both                   |

Without an icon or a color, those of the parent are used. These macros, and the ones that register events and bindings,
define a variable named `_`: use one per `.cpp` file. `ATMO_REGISTER_COMPONENT` has no such limit.

### Name it {#name}

Add the name of the type to `translation/en-US/atmo.json`, under `atmo.entities`. Without it, new entities are named
`atmo.entities.Entity::Entity2d::Coin2d#12`.

```json
"Entity::Entity2d::Coin2d": "Coin2d"
```

## The component {#component}

`ComponentMeta` describes the fields that are saved, shown in the inspector and available to the [[mcp-server]]. Fields
that are not listed, such as `collected`, only exist while the engine runs.

- `name` identifies the component in scene files. It must be unique: a second component with the same name is ignored.
- Keep the names of the fields stable. In a file, an unknown field stops the reading of its component without any message.
- Fields can be added: files without them keep the default value.

| Option                                        | Effect                                                                                     |
| --------------------------------------------- | ------------------------------------------------------------------------------------------ |
| `.withWidget("file_path")`                    | Uses that [widget](#widgets) in the inspector                                              |
| `.withRange(min, max)`                        | Gives the range of a number                                                                |
| `.withLabel`, `.withTooltip`, `.withCategory` | Describe the field                                                                         |
| `.asReadOnly()`                               | Cannot be changed through reflection                                                       |
| `.skipFlecs()`                                | Is not declared to Flecs. Needed for types it does not know, such as vectors and variants. |

`ATMO_REGISTER_COMPONENT_NO_FLECS` registers a type that is never a component of an entity, such as a subresource or a
section of the project settings.

### Widgets {#widgets}

These field types have a widget in the inspector:

| Type                                        | Widget                                      |
| ------------------------------------------- | ------------------------------------------- |
| `int`, `float`, `bool`, `std::string`       | `int`, `float`, `bool`, `string`            |
| `Vector2`, `Vector2i`, `Vector3`, `Vector4` | `vector2`, `vector2i`, `vector3`, `vector4` |
| `Color`                                     | `color_picker`                              |
| `std::vector` of a type with a widget       | `vector`                                    |

`file_path` and `subresource_list` are set by name or by the field type. Any other type, such as an enum, a `uint16_t` or a
structure, is shown without a way to change it. `Vector3i` and `Vector4i` name a widget that does not exist.

A new widget is registered with `ATMO_REGISTER_WIDGET(name, create, destroy, update)`: see `src/editor/widgets/`.
`DefaultWidget` selects it for a type.

## Systems {#systems}

- The same `RegisterSystems` runs for the game and for each scene editor. Return early when
  `WorldContext::is_editor_isolated` is set, for anything that must not happen while editing.
- Systems run while the world is read-only. To create or destroy entities, use `SignalQueue::Enqueue`: see
  [[signals-and-events#queue]].
- To free resources, add an observer for `flecs::OnRemove` on the component, as `Sprite2d` does.
- Project settings read in `RegisterSystems` are the default ones in run mode:
  see [[project-settings#late|late settings]]. Read them when the system runs instead.

## Children {#children}

An entity can create children in `initialize`. Children are saved with the scene, and created again when it is loaded: the
entity then has them twice. Avoid it for entities meant for scenes.

To reach a child, use its index in `getChildren`, whose order is kept, as `UiDropdownMenu::getLabel` does, or capture a
handle in the lambda that needs it. Do not look children up by name, and do not store a `flecs::entity` in a component.

## Check it {#check}

1. Build, and add the entity in the scene editor.
2. Save the scene, close its tab and open it again: the fields must keep their values.
3. Run the game.
4. Run `xmake test -v`.

To use the entity from scripts, see [[luau-bindings]]. Without a binding, scripts get the methods of its closest parent
that has one.
