---
id: physics
title: Physics
order: 11
description: Physics bodies and shapes, gravity and units, and how to move bodies.
---

# Physics

2D physics is simulated by [Box2D](https://box2d.org). An entity takes part in it when it is one of the three body types
and has at least one shape. The simulation only runs in the game, not in the scene editor.

## Bodies {#bodies}

| Entity          | Moved by                  | Use it for                                   |
| --------------- | ------------------------- | -------------------------------------------- |
| [[Static2d]]    | Nothing                   | Floors, walls                                |
| [[Dynamic2d]]   | Gravity, impulses, shocks | Balls, crates, characters driven by impulses |
| [[Kinematic2d]] | Its transform             | Moving platforms, doors                      |

A `Kinematic2d` is not affected by gravity or shocks, and does not collide with `Static2d` or with other `Kinematic2d`.

While the game runs, the simulation writes the position and rotation of the bodies it moves into their transform.

## Shapes {#shapes}

Shapes are in the `shapes` list of the **Body2dData** component. A body without a shape falls, but touches nothing and
does not react to impulses.

| Shape                | Field    | Default  |
| -------------------- | -------- | -------- |
| [[RectangleShape2d]] | `size`   | 50 by 50 |
| [[CircleShape2d]]    | `radius` | 2        |

Sizes are in pixels. Shapes are centered on the entity, and follow its rotation but not its `scale`: set the size of the
shape instead. A body can have several shapes, all with the same center.

In a scene or prefab file:

```json
"Body2dData": {
    "shapes": [
        {
            "type": "SubResource::Shape2d::RectangleShape2d",
            "data": { "size": { "x": 80, "y": 80 } }
        }
    ]
}
```

Density, friction and bounciness cannot be set in the editor: every shape uses Box2D's defaults, with a density of 1.

### Seeing the shapes {#debug}

The scene editor draws the shapes over the scene. This is the `draw_physics_debug`
[[project-settings#debug|project setting]].

## Units {#units}

Positions, sizes and velocities are in pixels, `x` to the right and `y` downwards. Physics works in meters, with
`pixels_per_meter` (50 by default) to convert:

| Quantity        | Unit                                                                       |
| --------------- | -------------------------------------------------------------------------- |
| Gravity         | m/s². The default, `y` = 9.81, accelerates bodies by 490 px/s².            |
| Mass            | Area in m² × density: 0.16 kg for a square of 20 pixels                    |
| Linear impulse  | kg·px/s: an impulse of 100 changes the velocity of that square by 625 px/s |
| Angular impulse | kg·m²/s                                                                    |

`gravity` is a [[project-settings#engine|project setting]] and applies to the whole scene. Its `z` is not used.

## Moving bodies {#moving}

Scripts move a `Dynamic2d` with impulses:

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

and a `Kinematic2d` by writing its transform:

```luau
function PhysicsUpdate(deltaTime)
    local transform = this:getTransform()
    if transform == nil then return end

    transform.position = Vector2(transform.position.x + 100 * deltaTime, transform.position.y)
end
```

Writing the transform of a body teleports it, whatever its type: it goes through obstacles, and does not give its speed
to what it touches. See [[luau-api#bodies]] for every method.

## Steps {#steps}

Physics advances by fixed steps of 1 / `physics_frame_rate` seconds, 60 per second by default, followed by
`PhysicsUpdate`. There is at most one step per frame: below 60 frames per second, physics runs slower than real time.

In each frame, physics comes after scripts are loaded and before `Update`.

## Parents {#parents}

Physics ignores the position, rotation and scale of the parents. A body under a parent at (1000, 0) is drawn 1000 pixels
away from where it collides. Place bodies directly in the scene, or under entities that stay at position (0, 0) without
rotation or scale. Giving children to a body, such as its sprite, is fine.

## Known issues {#known-issues}

- `OnCollisionEnter` is never called.
- `applyLinearVelocity` and `applyAngularVelocity` do not move bodies.
- In run mode, which includes the Play button, `physics_frame_rate` and `draw_physics_debug_in_game` are ignored: they
  are read before the project is opened. Physics runs at 60 steps per second, without the debug drawing.
- Pausing the game with the [[mcp-server]] does not pause physics.
