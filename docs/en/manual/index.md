---
id: manual
title: Manual
order: 2
description: How the engine works, system by system.
---

# Manual

The manual explains the engine's systems and how they work together. Start with the [[architecture]] overview.

- [[rendering]]: the GPU APIs the engine draws with and how one is chosen.
- [[screen-sizes]]: how scenes scale to the window and to HiDPI screens.
- [[shaders-and-materials]]: custom Slang shaders for sprites and post-processing.
- [[mcp-server]]: let an AI agent drive the editor or a running game.
- [[scripting]]: attach Luau scripts to entities, and the functions the engine calls.
- [[luau-api]]: every engine type and function available to scripts.
- [[prefabs]]: save entities to a file, and load them from the editor or from scripts.
- [[project-settings]]: every project setting and what it changes.
- [[page:entities]]: the entity types and the fields of their components.
- [[page:physics]]: bodies, shapes, units, and how to move bodies.
- [[page:ui]]: labels, buttons and their layout.
- [[texture-editor]]: draw images with layers and frames.
- [[exporting]]: turn a project into a standalone executable.
- [[addons]]: share assets, or extend the engine with a C++ library.

For contributors to the engine:

- [[adding-entities]]: declare and register an entity type and its component.
- [[luau-bindings]]: expose C++ types and functions to scripts.
- [[signals-and-events]]: notify other code, and change the world safely.
