---
title: ATMO Beta Test Plan
subtitle: BTP
author: Bary Théo, Flour de Saegher Albéric, Castres Raphael
module: G-EIP-700
version: 1.0
---

## **1. Project context**

This project focuses on the development of a video game engine designed to be powerful, efficient, and easy to use.

The main objective is to provide a complete tool that allows developers to create any type of game, with an initial focus on 2D games, while ensuring high performance and flexibility.

The engine is built to offer strong technical capabilities without sacrificing usability. It aims to handle complex game logic, rendering, and performance constraints, while remaining accessible to developers with different levels of experience.

To achieve this, the engine integrates multiple built-in creation and editing tools, including sprite and animation editors, directly within the engine. This approach reduces dependency on external software and provides a unified workflow for game development.

A significant part of the project is dedicated to UI/UX design. Special attention is given to the ergonomics, clarity, and overall user experience in order to make the engine intuitive, easy to understand, and efficient to use. The goal is to minimize friction during development and allow users to focus on creativity rather than tooling complexity.

Through this beta test, the project is presented in an advanced but non-final state.
The core features of the engine are implemented and functional, allowing users to explore its main workflows and capabilities. However, optimization, stability improvements, and usability refinements are still in progress.

This beta phase aims to assess the current stability, performance, and usability of the engine in real usage conditions, while gathering structured feedback from testers to identify issues, validate design choices, and guide future improvements.

## **2. User Roles**

The following roles will be involved in beta testing.

| **Role Name** | **Description**                                                                                                                                   |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| Game Dev      | Person that creates games using the engine. Can be single or a team working together.                                                             |
| Addon Dev     | Creates addons that can be loaded into the engine’s editor to add features. Can be LUAU/C++ code or regular assets.                               |
| Artist        | Artist that will draw textures and animations of the different entities of the game. And also test the rendering of his assets inside the engine. |

## **3. Feature table**

All of the listed features will be demonstrated during the beta presentation

| **Feature ID** | **User role** | **Feature name**     | **Short description**                                                                                                                                                                           |
| -------------- | ------------- | -------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| F1             | Game Dev      | Create game entities | Allows the game developer to create new entities within a scene. Entities act as containers for components and represent objects in the game world (player, enemies, environment, etc.).        |
| F2             | Game Dev      | Entities inspector   | Provides an inspector panel to add, remove, and configure components attached to an entity. Changes made through the inspector are reflected immediately in the game view.                      |
| F3             | Game Dev      | Program entity logic | Create a Script entity that will hold a LUAU script file, this file may hold code to add custom behaviors, executed at runtime.                                                                 |
| F4             | Game Dev      | Debug script errors  | Displays clear and structured error messages when LUAU scripts fail to load or execute. Errors include contextual information (file, line, message) to help developers quickly identify issues. |
| F5             | Everyone      | Start game           | Allows users to start and stop the game simulation directly from the editor to test gameplay and interactions in real time without leaving the editor environment.                              |
| F6             | Addon Dev     | Create addon         | Create a new addon from template with basic file structure                                                                                                                                      |
| F7             | Addon Dev     | Add LUAU context     | Addon loads through a LUAU file, it can expose methods and classes that will be used in all LUAU contexts. Also load built shared libraries                                                     |
| F8             | Everyone      | Project file system  | Explore project’s files through an integrated file explorer                                                                                                                                     |
| F9             | Everyone      | Create file from set | Can create a file corresponding to a certain file type. i.e. scripts or scenes                                                                                                                  |
| F10            | Everyone      | Delete file          | Delete a file from the file system through the integrated file explorer                                                                                                                         |
| F11            | Game Dev      | Export project       | Uses an export target corresponding to the editor’s version and packs all project data either in a dedicated packed file or directly inside the game’s executable                               |
| F12            | Artist        | Draw                 | Drawing tools to create 2D assets                                                                                                                                                               |
| F13            | Artist        | Import/export asset  | Import assets from outside the project or export what has been done                                                                                                                             |
| F14            | Artist        | Inspect assets       | Allow the artist to visualize what he has done with the drawing tools on the game directly                                                                                                      |

## **4. Success criteria**

| **Feature ID** | **Key success criteria**                                                            | **Indicator/metric**                                                                                                                                           | **Result** |
| -------------- | ----------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------- |
| F1             | The game developer can create multiple entities without performance loss or crashes | Create >= 100 entities in one scene<br>no crash<br>average >= 60 fps<br>entity creation fast < 100 ms                                                          |            |
| F2             | The game developer can modify entity components and see the result immediately      | component changes reflection < 5 frames<br>no restart required<br>0 desync between runtime and engine preview                                                  |            |
| F3             | LUAU scripts correctly alter entity behavior                                        | Script executes without runtime error<br>Behavior change visible within 5 frame after reload<br>>= 5 scripted entities running simultaneously without fps drop |            |
| F4             | Script errors are clearly reported and easy to understand                           | Error message contains file name, line number, and error type<br>Error appears in < 1s after execution<br>Engine does not crash on script error                |            |
| F5             | Users can start and stop the game reliably from the editor                          | Game starts in < 5 seconds<br>No crash when starting/stopping ≥10 times in a row<br>Game state resets correctly after stop                                     |            |
| F6             | Addon developer can create and extend addon templates                               | Addon template generated in < 5 seconds<br>≥5 addons loaded simultaneously<br>No editor crash or undefined behavior                                            |            |
| F7             | Addons can extend LUAU context and expose new features                              | Exposed functions/classes accessible in all LUAU contexts<br>Shared library loads successfully<br>No conflict between multiple addons                          |            |
| F8             | The user can see his files and navigates through the different files efficiently    | File tree loads in < 500 ms<br>No UI freeze when browsing >= 500 files                                                                                         |            |
| F9             | Users can create files                                                              | File appears < 500 ms in file system<br>File usable without manual fix                                                                                         |            |
| F10            | Users can delete files safely                                                       | File removed from disk and UI immediately<br>Confirmation dialog prevents accidental deletion                                                                  |            |
| F11            | Game can be exported and runs identically outside the editor                        | Export completes without error<br>Executable launches successfully<br>Behavior identical to editor runtime (logic, assets, performance)                        |            |
| F12            | Artists can draw and save 2D assets                                                 | Brush input latency < 50 ms<br>Saved image matches canvas state<br>No data loss after save/reload                                                              |            |
| F13            | Assets can be imported and exported correctly                                       | Supported formats load without distortion<br>Exported assets match editor output<br>Import/export completes in < 2 seconds for standard assets                 |            |
| F14            | Artists can preview assets in game directly without leaving asset editor            | Asset changes visible in-game without changing view<br>Update delay <= 5 frame<br>No need to restart scene or editor                                           |            |
