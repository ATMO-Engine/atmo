---
id: mcp-server
title: MCP server
order: 5
description: Let an AI agent drive the editor or a running game through the Model Context Protocol.
---

# MCP server

The editor binary (`atmo`) can run a [Model Context Protocol](https://modelcontextprotocol.io) server. An MCP client,
such as Claude Code, then edits scenes, materials and project settings, opens and closes editor tabs, takes viewport
screenshots, runs the game and presses keys in it, reads the log and evaluates Luau. Exported games
(`atmo-export`) never contain the server.

## Turning it on

The server is off by default. Either:

- start the engine with `--mcp [port]`, in the editor (`--project <path>`) or in run mode (`--project <path> --run`),
- or enable **MCP > enabled** in the project settings: the editor then starts the server every time it opens the
  project. The setting does not apply to run mode.

The port comes from `--mcp <port>`, else from **MCP > port** (27760 by default). The server listens on
`http://127.0.0.1:<port>/mcp` only, with the Streamable HTTP transport, and refuses requests whose `Origin` header is
not a local address.

To work without a window, add `--headless`. Headless mode uses the Null rendering platform, which draws nothing; add
`--rendering-platform=metal` (macOS) or `--rendering-platform=vulkan` when the agent needs screenshots:

```sh
atmo --headless --rendering-platform=metal --project path/to/project --mcp
```

Headless mode runs at 60 frames per second instead of as fast as possible.

## Connecting Claude Code

Start the engine first, then register the server once:

```sh
claude mcp add --transport http atmo http://127.0.0.1:27760/mcp
```

or add it to the project's `.mcp.json`:

```json
{
    "mcpServers": {
        "atmo": { "type": "http", "url": "http://127.0.0.1:27760/mcp" }
    }
}
```

The client connects when it starts; if the engine was started later, reconnect with `/mcp`.

## Editor tools

Paths are relative to the project root; `project://` paths and absolute paths work too. Tools that work on a scene
take its file in `scene` and open a scene editor tab for it when none is open. Those tabs are marked `opened_by_mcp`,
and `editor_close_auto` closes the ones without unsaved changes. Entities are addressed by their path below the scene
root (`Player/Sprite`) or by the numeric id other tools return; `/` is the scene root.

| Tool                                                                 | What it does                                                                   |
| -------------------------------------------------------------------- | ------------------------------------------------------------------------------ |
| `editor_types`, `editor_list`                                        | Editor types with their file extensions; open tabs (id, path, dirty, active).  |
| `editor_open`                                                        | Open a file in the editor of its extension, or focus its tab; `create` a file. |
| `editor_focus`, `editor_save`, `editor_close`, `editor_close_auto`   | Tab management. `editor_close` refuses to drop unsaved changes by default.     |
| `command_list`, `command_run`                                        | The editor commands (menus and shortcuts), except the ones that open dialogs.  |
| `entity_types`, `component_types`                                    | What `entity_create` and `component_set` accept, with component fields.        |
| `scene_tree`, `entity_get`                                           | The entity tree; one entity with all its component values.                     |
| `entity_create`, `entity_delete`, `entity_rename`, `entity_reparent` | Change the tree.                                                               |
| `component_set`, `component_remove`                                  | Change component values (JSON merge patch) or remove a component.              |
| `entity_select`                                                      | Select an entity in the scene editor's tree and inspector.                     |
| `prefab_instantiate`, `prefab_save`, `scene_save`                    | Prefab files and saving.                                                       |
| `scene_screenshot`                                                   | PNG of a scene editor viewport, with an optional size, center and zoom.        |
| `project_info`, `project_settings_get`, `project_settings_set`       | Project description and settings.                                              |
| `asset_tree`                                                         | The project's files.                                                           |
| `material_get`, `material_set`, `material_preview`, `shader_status`  | See [[shaders-and-materials]]: parameters, values, a rendered preview, errors. |
| `play_start`, `play_stop`, `play_status`, `game_tools`, `game_call`  | Run the game and call its tools (below).                                       |
| `luau_eval`, `logs_read`                                             | Evaluate Luau, read the log.                                                   |

The tab's dirty flag only tracks changes made through MCP tools; changes made with the mouse are not counted, so
`editor_close` and `editor_close_auto` can close a tab with unsaved manual edits.

## Running the game

`play_start` saves the modified tabs, then starts the game in a child process with `--mcp <port + 1>` (and
`--headless` when asked). `game_call` forwards a tool call to it and `game_tools` lists its tools. Starting the game
directly with `atmo --project <path> --run --mcp` gives the same tools without the editor:

| Tool                        | What it does                                                                                   |
| --------------------------- | ---------------------------------------------------------------------------------------------- |
| `game_status`               | Frame, game time, pause state and the input names scripts use.                                 |
| `game_pause`, `game_resume` | A paused game keeps running its systems with a delta time of 0.                                |
| `game_step`                 | Pause, run some frames with a fixed delta time (1/60 s by default), then answer.               |
| `game_input`                | Key, mouse button, wheel and text events, frames apart; a paused game is stepped through them. |
| `game_action`               | Hold a named input (for example `Space`) for some frames.                                      |
| scene tools, `luau_eval`    | Same as in the editor, on the running scene: leave `scene` out.                                |

Input goes through the same path as the window's events, so `Input.isJustPressed` sees it in the next frame.

## Logs and Luau

`logs_read` returns the last 2000 log entries (UTC times), from a sequence number on, so the agent can read only what
is new. Luau's `print` writes to the log.

`luau_eval` runs a Luau chunk in a new sandboxed thread of the scripting VM and returns its results as strings, the
lines it printed and the log entries it produced. `entity` binds an entity to the global `this`, like in an entity
script. The chunk can do anything a script can, including breaking the running state.
