---
id: prefabs
title: Prefabs
order: 8
description: Save an entity and its children to a file, and load copies of it from the editor or from scripts.
---

# Prefabs

A prefab is an entity saved to a file with its components and all its children. It can be loaded as many times as needed,
in the editor or while the game runs: enemies, projectiles, pickups...

Prefabs and scenes both use the `.ascn` extension, but are not interchangeable: the root of a scene file is a `Scene`,
the root of a prefab is any other entity. A prefab cannot be the [[projects#default-scene|default scene]], and a scene
cannot be loaded as a prefab.

|                 | In the editor                 | In a script                          | With the MCP server          |
| --------------- | ----------------------------- | ------------------------------------ | ---------------------------- |
| Create a prefab | [Save button](#create-editor) | Not possible                         | [`prefab_save`](#mcp)        |
| Load a prefab   | [Load button](#load-editor)   | [`Prefab.instantiate`](#load-script) | [`prefab_instantiate`](#mcp) |

## Create a prefab in the editor {#create-editor}

1. In the scene editor, build the entity: its components, its children and, if needed, its script.
2. On its row in the entity tree, press the save button (the disk).
3. Choose a file in the project, for example `assets/prefabs/enemy.ascn`. The extension is added when missing.

The file contains the entity as it was when the button was pressed, including its position. The entity stays in the scene,
and is not linked to the file.

Use [[projects#project-paths|project:// paths]] in the components, such as `script_path` and `texture_path`, so that
the prefab works on other computers and in exported games.

### Names {#names}

Entities created in the editor have names such as `Dynamic2d#709`, and the scene editor cannot rename them. To use the
name in scripts, with `getChild` for example, change `name` in the file with a text editor:

```json
{
    "name": "Enemy",
    "type": "Entity::Entity2d",
```

::: warning
Give a different name to each entity of the prefab. Two children of the same entity with the same name, or a child named
like one of its parents, currently make the engine crash when the prefab is loaded. Names can be reused in other prefabs,
and the same prefab can be loaded several times.
:::

### Write a prefab by hand {#by-hand}

A prefab is a JSON file, so it can also be written in a text editor:

```json
{
    "name": "Enemy",
    "type": "Entity::Entity2d",
    "components": {
        "Script": { "script_path": "project://assets/script/enemy.luau" },
        "Transform2d": {
            "position": { "x": 0, "y": 0 },
            "rotation": 0,
            "scale": { "x": 1, "y": 1 }
        }
    },
    "children": [
        {
            "name": "Body",
            "type": "Entity::Entity2d::Sprite2d",
            "components": {
                "Sprite2d": { "texture_path": "project://assets/enemy.png" }
            },
            "children": []
        }
    ]
}
```

`type` is the full name of the entity type. Components and fields left out keep their default value. To see the names of
a type and its components, save an entity of that type from the editor and look at the file.

## Load a prefab in the editor {#load-editor}

1. On the row of the entity that will be the parent, press the load button (the open folder).
2. Choose the prefab file.

The prefab is added to the children of that entity. The scene itself has no row: to put prefabs at the top of the scene,
add an `Entity2d` that stays at position (0, 0) and load them in it.

The scene then contains a copy. Save the scene to keep it.

## Load a prefab in a script {#load-script}

`Prefab.instantiate` loads a prefab while the game runs:

```luau
Prefab.instantiate("project://assets/prefabs/enemy.ascn", this:getParent(), function(enemy)
    local transform = enemy:getTransform()
    if transform ~= nil then
        transform.position = Vector2(200, 100)
    end
end)
```

| Parameter  | Description                                                                          |
| ---------- | ------------------------------------------------------------------------------------ |
| `path`     | The prefab file.                                                                     |
| `parent`   | The entity that receives the copy. Required.                                         |
| `callback` | Optional function, called with the root entity of the copy once it has been created. |

`this:getParent()` puts the copy next to the entity of the script, and `this` puts it inside, so that it follows the
entity.

### When the copy exists {#timing}

`Prefab.instantiate` returns nothing and does not create the copy immediately:

1. The copy is created at the end of the frame.
2. The callback is called right after.
3. The `Create` of the copy's own scripts is called in the next frame.

Values set in the callback are therefore in place when `Create` runs. Code written after `Prefab.instantiate` cannot use
the copy yet. To use it later, keep it in a variable, and check that it is still there:

```luau
local enemy = nil

function Create()
    Prefab.instantiate("project://assets/prefabs/enemy.ascn", this:getParent(), function(entity)
        enemy = entity
    end)
end

function Update(deltaTime)
    if enemy ~= nil and enemy:isAlive() then
        print(enemy:name())
    end
end
```

### Spawn at a regular interval {#spawner}

```luau
local INTERVAL = 2
local timer = 0

local function spawn()
    Prefab.instantiate("project://assets/prefabs/enemy.ascn", this:getParent(), function(enemy)
        local transform = enemy:getTransform()
        local origin = this:getTransform()
        if transform == nil or origin == nil then return end

        transform.position = Vector2(origin.position.x + math.random(-200, 200), origin.position.y)
    end)
end

function Update(deltaTime)
    timer += deltaTime
    if timer >= INTERVAL then
        timer -= INTERVAL
        spawn()
    end
end
```

Attach this script to an entity with a transform. Scripts cannot destroy entities yet, so limit how many copies are
created.

### Several copies {#copies}

Each copy has its own components and runs its own copy of the scripts: their variables are not shared. The root gets the
name saved in the file, then a number if the parent already has a child with that name: `Enemy`, `Enemy_2`, `Enemy_3`...
Children keep their name.

### Prefabs with physics {#physics}

Physics ignores the position of the parents: a body under a parent at (1000, 0) is drawn 1000 pixels away from where it
collides. Load prefabs with a `Static2d`, `Kinematic2d` or `Dynamic2d` under the scene, or under entities that stay at
position (0, 0). With `this:getParent()`, this means that the parent of the script's entity is the scene or such an
entity.

## Change a prefab {#change}

Copies are not linked to the file. Changing the file changes the copies loaded afterwards, and not the ones already saved
in scenes.

To change a prefab, load it in a scene, edit the copy, then save it to the same file. Or edit the file in a text editor.

::: danger
Do not open a prefab file as a scene in the editor. It cannot be loaded as one, so the tab is empty, and saving that tab
replaces the prefab with an empty scene.
:::

## With the MCP server {#mcp}

An AI agent connected to the [[mcp-server]] can save and load prefabs in a scene. In the running game, omit `scene`.

| Tool                 | Arguments                 | Notes                                                  |
| -------------------- | ------------------------- | ------------------------------------------------------ |
| `prefab_save`        | `scene`, `entity`, `path` | Creates the missing folders and adds the extension.    |
| `prefab_instantiate` | `scene`, `path`, `parent` | Without `parent`, the copy is added to the scene root. |
| `entity_rename`      | `scene`, `entity`, `name` | Gives an entity a useful name before saving it.        |

## Errors {#errors}

When a prefab cannot be loaded, the reason is logged and the callback is not called.

| Message                                               | Cause                                                                 |
| ----------------------------------------------------- | --------------------------------------------------------------------- |
| `File not found in packed file system`                | [[projects#not-found\|The path is wrong]].                            |
| `is a scene and can't be instantiated under a parent` | The file is a scene. Save the entity itself as a prefab.              |
| `is a prefab, not a scene`                            | The file is a prefab, used as the default scene or opened as a scene. |
| `Failed to parse`                                     | The file is not valid JSON.                                           |
| `Entity expected`                                     | `parent` is missing or `nil`.                                         |
