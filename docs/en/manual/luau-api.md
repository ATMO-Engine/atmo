---
id: luau-api
title: Luau API
order: 7
description: Every engine type and function available to Luau scripts.
---

# Luau API

Everything the engine adds to Luau. To learn how scripts run, start with [[scripting]].

Methods are called with `:` (`this:name()`), and the functions of `Input` and `Prefab` with `.` (`Input.isPressed("D")`).
In signatures, `?` marks an optional parameter or a result that can be `nil`.

## Globals {#globals}

| Name                | Description                                                      |
| ------------------- | ---------------------------------------------------------------- |
| `this`              | The entity the script is attached to: see [entities](#entities). |
| `print(...)`        | Writes its arguments to the log.                                 |
| `Vector2(x, y)`     | Creates a [Vector2](#vector2).                                   |
| `Color(r, g, b, a)` | Creates a [Color](#color).                                       |
| `Transform2d()`     | Creates a [Transform2d](#transform2d) that belongs to no entity. |
| `Input`             | Keyboard and mouse: see [Input](#input).                         |
| `Prefab`            | Entity creation: see [Prefab](#prefab).                          |

The Luau libraries `coroutine`, `table`, `string`, `math`, `utf8`, `bit32`, `buffer` and `vector` are available, as well
as the base functions (`pcall`, `pairs`, `tostring`...). `require`, `io`, `os`, `debug`, `task` and `wait` are not.
Libraries and engine types cannot be modified by scripts.

## Vector2 {#vector2}

```luau
local v = Vector2(3, 4)
print(v.x, v.y, v:length()) -- 3 4 5
```

| Member             | Description                      |
| ------------------ | -------------------------------- |
| `x: number`        | Horizontal component, read-only. |
| `y: number`        | Vertical component, read-only.   |
| `length(): number` | Length of the vector.            |

Both numbers are required. A `Vector2` never changes: create a new one instead. It has no operators, so add the
components yourself, and compare them too, because `Vector2(1, 2) == Vector2(1, 2)` is `false`:

```luau
local sum = Vector2(a.x + b.x, a.y + b.y)
local equal = a.x == b.x and a.y == b.y
```

## Color {#color}

```luau
local halfRed = Color(1, 0, 0, 0.5)
```

`r`, `g`, `b` and `a` are numbers from 0 to 1, and are read-only. All four are required. Like `Vector2`, a `Color` has no
operators.

## Transform2d {#transform2d}

Place of an entity, given by [`getTransform`](#entity). Writing a property changes the entity.

| Property     | Type                  | Description                                  |
| ------------ | --------------------- | -------------------------------------------- |
| `position`   | [`Vector2`](#vector2) | Position relative to the parent, in pixels.  |
| `rotation`   | `number`              | Rotation relative to the parent, in degrees. |
| `scale`      | [`Vector2`](#vector2) | Scale relative to the parent.                |
| `g_position` | [`Vector2`](#vector2) | Position in the scene.                       |
| `g_rotation` | `number`              | Rotation in the scene.                       |
| `g_scale`    | [`Vector2`](#vector2) | Scale in the scene.                          |

```luau
local transform = this:getTransform()
transform.position = Vector2(100, 50)
transform.rotation = transform.rotation + 90
```

- The `g_` properties are computed from the others on every frame. Writing them has no lasting effect.
- `g_position` adds the positions of the parents. It does not take their rotation or scale into account.
- Writing a field of a vector, as in `transform.position.x = 1`, [[scripting#copy-error|is an error]].
- Using the transform of a destroyed entity raises an error.

## Entities {#entities}

Scripts get an entity from `this`, `getChild`, `getParent` or `Prefab.instantiate`. Its methods depend on its type:

| Entity type                                                          | Methods                                      |
| -------------------------------------------------------------------- | -------------------------------------------- |
| `Sprite2d`                                                           | [Sprite2d](#sprite2d) and [Entity](#entity)  |
| `Kinematic2d`                                                        | [Kinematic2d](#bodies) and [Entity](#entity) |
| `Dynamic2d`                                                          | [Dynamic2d](#bodies) and [Entity](#entity)   |
| Every other type (`Entity2d`, `Static2d`, `Camera2d`, UI, scenes...) | [Entity](#entity)                            |

An entity can be destroyed while a script still holds it: check `isAlive` before using an entity kept in a variable.
Fields cannot be added to an entity. Two values for the same entity are not equal with `==`.

### Entity {#entity}

| Method                            | Description                                                                    |
| --------------------------------- | ------------------------------------------------------------------------------ |
| `name(): string?`                 | Name of the entity.                                                            |
| `isAlive(): boolean`              | Whether the entity still exists.                                               |
| `getTransform(): Transform2d?`    | Its [Transform2d](#transform2d), or `nil` if it has none, such as a UI entity. |
| `getChild(name: string): Entity?` | Child with that name, or `nil`.                                                |
| `getParent(): Entity?`            | Parent of the entity, or `nil`.                                                |

`name`, `getTransform`, `getChild` and `getParent` return `nil` when the entity has been destroyed.

`getChild` only looks at direct children. Separate names with `::` to go further down:

```luau
local hand = this:getChild("Arm::Hand")
if hand ~= nil then
    print(hand:name())
end
```

### Sprite2d {#sprite2d}

| Method                                                     | Description                                                     |
| ---------------------------------------------------------- | --------------------------------------------------------------- |
| `getMaterial(): Material?`                                 | [Material](#material) of the sprite, or `nil`.                  |
| `setMaterial(path: string?)`                               | Loads a `.material` file. Without a path, removes the material. |
| `getModulate(): Color`                                     | Tint of the sprite. White by default.                           |
| `setModulate(color: Color)`                                | Changes the tint.                                               |
| `getCustom(): (number, number, number, number)`            | The four numbers given to the shader.                           |
| `setCustom(x: number, y: number?, z: number?, w: number?)` | Changes them. Missing numbers are 0.                            |

```luau
this:setMaterial("project://assets/wobble.material")
this:setModulate(Color(1, 0.5, 0.5, 1))
this:setCustom(0.2)
```

If the file cannot be loaded, `setMaterial` logs an error and the sprite has no material. See
[[shaders-and-materials#materials]] for what these values do.

### Kinematic2d and Dynamic2d {#bodies}

Both types have the same methods:

| Method                                                 | Description                                                            |
| ------------------------------------------------------ | ---------------------------------------------------------------------- |
| `getLinearVelocity(): Vector2?`                        | Velocity, in pixels per second.                                        |
| `getAngularVelocity(): number?`                        | Angular velocity, in degrees per second.                               |
| `applyLinearImpulse(impulse: Vector2, wake: boolean?)` | Pushes the body at its center: the velocity changes by impulse / mass. |
| `applyAngularImpulse(impulse: number, wake: boolean?)` | Makes the body turn, in kg·m²/s.                                       |
| `applyLinearVelocity(velocity: Vector2)`               | Replaces the velocity. [Has no effect yet](#velocity-issue).           |
| `applyAngularVelocity(velocity: number)`               | Replaces the angular velocity. [Has no effect yet](#velocity-issue).   |

`wake` is `true` by default, and wakes up a body that has come to rest. The unit of linear impulses is kg·px/s.

Impulses have no effect on a `Kinematic2d`, because physics gives it no mass. `Static2d` has none of these methods.

#### Velocity setters {#velocity-issue}

::: warning
`applyLinearVelocity` and `applyAngularVelocity` currently do not move the body, even when `getLinearVelocity` then
returns the new value. Move a `Dynamic2d` with impulses, and a `Kinematic2d` with its transform:

```luau
function PhysicsUpdate(deltaTime)
    local transform = this:getTransform()
    local position = transform.position
    transform.position = Vector2(position.x + 100 * deltaTime, position.y)
end
```

:::

## Material {#material}

Given by a sprite's [`getMaterial`](#sprite2d). A material is shared: a change applies to every sprite that uses it. To
vary a single sprite, use `setCustom` or `setModulate`.

| Method                              | Description                                                 |
| ----------------------------------- | ----------------------------------------------------------- |
| `set(name: string, value): boolean` | Changes a shader parameter. Returns whether it was changed. |
| `get(name: string): value?`         | Value of a shader parameter, or `nil`.                      |
| `getShader(): string`               | Path of the shader.                                         |

Values depend on the type of the [[shaders-and-materials#parameters|parameter]]:

| Parameter        | `set` takes                                      | `get` returns    |
| ---------------- | ------------------------------------------------ | ---------------- |
| A number         | `number`, or a `boolean` (1 or 0)                | `number`         |
| `float2`         | `Vector2` or `{ x, y }`                          | `Vector2`        |
| `float3`         | `{ x, y, z }`                                    | Table of numbers |
| `float4`         | `{ x, y, z, w }` or `Color`                      | Table of numbers |
| `[Color] float4` | `Color` or `{ r, g, b, a }`                      | `Color`          |
| Texture          | Path of the image, such as `project://noise.png` | Path             |

```luau
local material = this:getMaterial()
if material ~= nil then
    material:set("strength", 0.02)
    material:set("tint", Color(1, 0, 0, 1))
    material:set("noise", "project://assets/noise.png")
end
```

When the parameter does not exist, or the value has the wrong number of components, `set` logs a warning and returns
`false`. A value of another type, such as `nil`, raises an error.

## Input {#input}

| Function                                | Description                                                                          |
| --------------------------------------- | ------------------------------------------------------------------------------------ |
| `isPressed(name: string): boolean`      | Whether the input is held.                                                           |
| `isJustPressed(name: string): boolean`  | Whether the input was pressed in this frame.                                         |
| `isReleased(name: string): boolean`     | Whether the input is up after having been pressed. `false` before the first press.   |
| `isJustReleased(name: string): boolean` | Whether the input was released in this frame. Always `false` for keys at the moment. |
| `getMousePosition(): Vector2`           | Mouse position in the window. It does not follow the camera.                         |

To detect that a key is up, prefer `not Input.isPressed(name)`. Holding a key can make `isJustPressed` return `true`
again, because of the system's key repeat.

### Input names {#input-names}

| Name            | Input              |
| --------------- | ------------------ |
| `D`             | <kbd>D</kbd>       |
| `Q`             | <kbd>Q</kbd>       |
| `S`             | <kbd>S</kbd>       |
| `Z`             | <kbd>Z</kbd>       |
| `Space`         | <kbd>Space</kbd>   |
| `ui_click`      | Left mouse button  |
| `ui_rightClick` | Right mouse button |
| `ui_confirm`    | <kbd>Enter</kbd>   |
| `ui_quit`       | <kbd>Escape</kbd>  |

Names are case-sensitive. An unknown name raises `Input not found`. Letters follow the keyboard layout: `Q` is the key
that types Q.

## Prefab {#prefab}

| Function                                                         | Description                            |
| ---------------------------------------------------------------- | -------------------------------------- |
| `instantiate(path: string, parent: Entity, callback: function?)` | Creates the entities of a prefab file. |

- `path` is a prefab saved from the scene editor as an `.ascn` file. Scene files are refused.
- `parent` is the entity that receives the new one. It is required.
- `callback` receives the root entity of the prefab once it is created.

The function returns nothing: the prefab is created at the end of the frame, so only the callback can use it. The root
gets the name saved in the file, followed by a number if the parent already has a child with that name (`Enemy`,
`Enemy_2`...).

```luau
Prefab.instantiate("project://assets/prefabs/enemy.ascn", this:getParent(), function(enemy)
    local transform = enemy:getTransform()
    if transform ~= nil then
        transform.position = Vector2(200, 100)
    end
end)
```

If the file cannot be loaded, an error is logged and the callback is not called.

To create prefab files, and for more about loading them, see [[prefabs]].
