---
id: architecture
title: Architecture overview
order: 1
description: The engine's layers, from the main loop down to resources, input and addons.
---

# Architecture overview

ATMO is a C++23 cross-platform 2D game engine built around an Entity Component System using
[Flecs](https://www.flecs.dev/).

## The engine {#engine}

[[core::Engine]] owns the main loop. It initializes SDL, input and logging, then drives the ECS world every frame.
Entities are created and destroyed through the [[EntityRegistry]], and every scene is rooted in a
[[ecs::entities::Scene]] entity that manages the lifecycle of its children.

## Core systems {#core-systems}

| System              | Responsibility                                                 |
| ------------------- | -------------------------------------------------------------- |
| [[ResourceManager]] | Handle-based loading and caching of images, fonts and scripts  |
| [[InputManager]]    | Keyboard, joystick and controller input through SDL3           |
| [[EventRegistry]]   | Observer-style event dispatch                                  |
| [[ProjectManager]]  | Reads `.atmo` project files and mounts the virtual file system |
| [[MetaRegistry]]    | Runtime type information used for serialization and the editor |

## Addons {#addons}

Addons are declared with an `addon.json` manifest. An [[Addon]] whose `atmo_version` does not match the running
engine is rejected with [[Addon::AddonLoadException]]. Addons may ship a native library exporting an entry point
declared with [[ATMO_REGISTER_ADDON]], which receives an [[AtmoAPI]] giving access to the engine's core systems.

To keep this page accurate, every symbol above is a reference checked at build time: see
[[writing-documentation#references]].
