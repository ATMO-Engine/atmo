---
id: entities
title: Entities and components
order: 10
description: What each entity type is for, and the component fields it has.
---

# Entities and components

A scene is a tree of entities. The type of an entity decides which components it starts with, and components hold its
data: this is what the inspector shows and what scene files store.

## Entity types {#types}

| Type                   | Components                                         | Purpose                                               |
| ---------------------- | -------------------------------------------------- | ----------------------------------------------------- |
| [[Entity]]             | None                                               | Groups other entities                                 |
| [[Entity2d]]           | [Transform2d](#transform2d)                        | Something with a place in the world                   |
| [[entities::Sprite2d]] | [Transform2d](#transform2d), [Sprite2d](#sprite2d) | Draws an image                                        |
| [[Camera2d]]           | [Transform2d](#transform2d), [CameraData](#camera) | Chooses the part of the world that is shown           |
| [[Static2d]]           | [Transform2d](#transform2d), [Body2dData](#body)   | Physics body that does not move: see [[page:physics]] |
| [[Dynamic2d]]          | [Transform2d](#transform2d), [Body2dData](#body)   | Physics body moved by the simulation                  |
| [[Kinematic2d]]        | [Transform2d](#transform2d), [Body2dData](#body)   | Physics body moved by its transform                   |
| UI types               | `UI`, `Layout` and others                          | Interface elements: see [[page:ui]]                   |

[[Body2d]] cannot be created: it only groups the three kinds of bodies. [[entities::Scene]] and [[entities::Window]] are
created by the engine: do not add them to a scene. The list of the editor also shows the types it uses for its own interface, such as `UIPanel`,
`UIGrid` or `UIFileExplorer`. They do not exist in exported games.

Every entity can also have a [Script](#script).

In scene files and tools, a type is written with its parents, as in `Entity::Entity2d::Sprite2d`.

## Transform2d {#transform2d}

| Field      | Default | Description                                 |
| ---------- | ------- | ------------------------------------------- |
| `position` | (0, 0)  | Position relative to the parent, in pixels  |
| `rotation` | 0       | Rotation relative to the parent, in degrees |
| `scale`    | (1, 1)  | Scale relative to the parent                |

Positions and rotations are added to those of the parents, and scales are multiplied. The position of a child does not
turn or scale with its parent: a child at (10, 0) of a parent that is rotated by 90° stays 10 pixels to its right.

## Sprite2d {#sprite2d}

| Field           | Default      | Description                                                  |
| --------------- | ------------ | ------------------------------------------------------------ |
| `texture_path`  | Empty        | Image to draw, centered on the entity, at its size in pixels |
| `modulate`      | White        | Color the image is multiplied by, including its opacity      |
| `material_path` | Empty        | [[shaders-and-materials#materials\|Material]] to draw with   |
| `custom`        | (0, 0, 0, 0) | Four numbers for the material                                |

Use [[projects#project-paths|project:// paths]]. A sprite without a texture draws nothing. Textures are smooth by
default; for pixel art, see [[screen-sizes#texture-filter]].

## CameraData {#camera}

| Field          | Default | Description                                                                    |
| -------------- | ------- | ------------------------------------------------------------------------------ |
| `zoom`         | 1       | 2 shows everything twice as large                                              |
| `is_active`    | Yes     | Whether the game looks through this camera                                     |
| `post_process` | Empty   | [[shaders-and-materials#post-processing\|Post-processing materials]], in order |

The camera shows its position at the center of the window. Without an active camera, the point (0, 0) of the world is in
the top-left corner.

- Only one camera should be active. With several, one of them is used.
- The rotation and scale of a camera are ignored, as well as the position of its parents: placing a camera inside the
  player does not make it follow the player. Copy the position in a script instead:

```luau
function Update(deltaTime)
    local player = this:getParent():getChild("Player")
    if player == nil then return end

    local camera = this:getTransform()
    local target = player:getTransform()
    if camera == nil or target == nil then return end

    camera.position = target.g_position
end
```

This script is for a camera next to an entity named `Player`. Scripts cannot change the zoom or the active camera.

## Body2dData {#body}

| Field    | Description                                               |
| -------- | --------------------------------------------------------- |
| `shapes` | Collision shapes of the body: see [[page:physics#shapes]] |

## Script {#script}

| Field         | Description                                  |
| ------------- | -------------------------------------------- |
| `script_path` | Luau script of the entity: see [[scripting]] |

Add it with **Add Script** in the inspector.

## Names {#names}

An entity created in the editor is named after its type and a number, as in `Sprite2d#12`. The scene editor cannot rename
entities: change `name` in the scene file with a text editor, while the scene is closed, or use the `entity_rename` tool
of the [[mcp-server]]. Two entities with the same parent need different names.

## Scene files {#scene-files}

A scene is an `.ascn` file, in JSON:

```json
{
    "name": "Main",
    "type": "Entity::Scene",
    "components": { "Scene": { "singleton": false } },
    "children": [
        {
            "name": "Player",
            "type": "Entity::Entity2d::Sprite2d",
            "components": {
                "Sprite2d": { "texture_path": "project://assets/player.png" },
                "Transform2d": { "position": { "x": 200, "y": 100 } }
            },
            "children": []
        }
    ]
}
```

- Missing components and fields keep their default value.
- Unknown components are ignored. An unknown field stops the reading of its component without any message: the fields
  written after it keep their default value.
- Prefabs use the same format: see [[prefabs]].
