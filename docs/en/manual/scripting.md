---
id: scripting
title: Scripting
order: 6
description: Attach Luau scripts to entities and use the functions the engine calls.
---

# Scripting

Game logic is written in [Luau](https://luau.org), in scripts attached to entities. There is no main script: a game
starts with its [[projects#default-scene|default scene]], and each script of that scene runs for its own entity.

This page explains how a script runs. Everything a script can use is listed in [[luau-api]].

## A first script {#first-script}

1. In the project folder, create `assets/script/player.luau`:

    ```luau
    local SPEED = 100

    function Create()
        print("Hello from " .. this:name())
    end

    function Update(deltaTime)
        local transform = this:getTransform()
        if transform == nil then return end

        local position = transform.position
        if Input.isPressed("D") then
            transform.position = Vector2(position.x + SPEED * deltaTime, position.y)
        elseif Input.isPressed("Q") then
            transform.position = Vector2(position.x - SPEED * deltaTime, position.y)
        end
    end
    ```

2. In the scene editor, select an entity with a transform, for example a `Sprite2d`.
3. In the inspector, click **Add Script** and write `project://assets/script/player.luau` in `script_path`. See
   [[projects#project-paths]].
4. Save the scene, then press Play. The scene must be the project's [[projects#default-scene|default scene]].

Scripts only run in the game. The scene editor compiles them, to show their errors, but does not run them. A change made
to a script is used the next time the game starts.

## Entry points {#entry-points}

The engine calls the global functions below when the script defines them. All of them are optional, and the names must be
written exactly like this.

| Function                   | Called                                                                       |
| -------------------------- | ---------------------------------------------------------------------------- |
| `Create()`                 | Once, when the script has been loaded.                                       |
| `Update(deltaTime)`        | Once per frame. `deltaTime` is the time in seconds since the previous frame. |
| `PhysicsUpdate(deltaTime)` | After each physics step. `deltaTime` is the fixed duration of a step.        |
| `OnCollisionEnter(other)`  | Reserved for collisions, [not called yet](#known-issues).                    |

Do not declare them `local`: the engine could not find them.

### The body of the script {#script-body}

Code outside of any function runs once, before `Create`. Use it to declare variables and constants. If it raises an
error, the script stops and none of its functions are called.

### Create {#create}

The place to prepare the entity. Its `g_position`, `g_rotation` and `g_scale` are not computed yet: they are from the end
of the first frame.

### Update {#update}

The place to read inputs and move entities without physics. Multiply speeds by `deltaTime`, so that the game runs at the
same speed whatever the frame rate.

### PhysicsUpdate {#physics-update}

The place to apply impulses. `deltaTime` is 1 / `physics_frame_rate`, a project setting that is 60 by default.
`PhysicsUpdate` is not called on every frame, so read inputs such as `Input.isJustPressed` in `Update`, and remember
them:

```luau
local jump = false

function Update(deltaTime)
    if Input.isJustPressed("Space") then
        jump = true
    end
end

function PhysicsUpdate(deltaTime)
    if jump then
        jump = false
        this:applyLinearImpulse(Vector2(0, -100))
    end
end
```

## this {#this}

`this` is the entity the script is attached to. What it offers depends on the type of the entity: see
[[luau-api#entities]].

## Variables and other scripts {#variables}

Each entity runs its own copy of the script. Neither local nor global variables are shared, whether between two entities
with the same script or between two scripts. `require` is not available: a script cannot load another one.

## Coordinates and units {#units}

| Quantity         | Unit                                                                               |
| ---------------- | ---------------------------------------------------------------------------------- |
| Position         | Pixels. `x` grows to the right and `y` grows downwards: jumping is a negative `y`. |
| Linear velocity  | Pixels per second                                                                  |
| Rotation         | Degrees                                                                            |
| Angular velocity | Degrees per second                                                                 |
| Time             | Seconds                                                                            |

Physics converts pixels to meters with the `pixels_per_meter` project setting, 50 by default.

## Errors {#errors}

Errors are written to the log, with the name of the script and the line:

- **Compile error**: the script is not loaded. The scene editor reports it as soon as the script is attached.
- **Error in the body of the script**: the script stops.
- **Error in an entry point**: that call stops, and the engine calls the function again at the next frame or step. An
  error in `Update` is therefore logged on every frame.

`print` also writes to the log. Use `pcall` to handle an error in the script.

### This value is a copy {#copy-error}

```text
'x' can't be modified, this value is a copy: create a new one instead (e.g. transform.position = Vector2(x, y))
```

Reading `transform.position` gives a copy, so changing one of its fields would have no effect. Write the whole value
instead:

```luau
transform.position.x = 10                               -- error
transform.position = Vector2(10, transform.position.y)  -- correct
```

## Known issues {#known-issues}

- `OnCollisionEnter` is never called.
- `applyLinearVelocity` and `applyAngularVelocity` do not move bodies. Use impulses on a `Dynamic2d`, and write
  `transform.position` on a `Kinematic2d`.
- `Input.isJustReleased` is always `false` for keys. It works for mouse buttons.
- Only a few keys can be read: see [[luau-api#input-names]].
- Prefabs with repeated names crash the engine: see [[prefabs#names|the names of prefabs]].
- Saving a prefab that was opened as a scene erases it: see [[prefabs#change]].
- Some project settings are ignored in run mode: see [[project-settings#late]].
- Scenes saved with some UI widgets cannot be opened again: see
  [[page:ui#known-issues|the known issues of the user interface]].
