---
id: project-settings
title: Project settings
order: 9
description: Every project setting, its default value and what it changes.
---

# Project settings

Settings are stored in `.atmo/project_settings.json`, in the [[projects#layout|project folder]]. Change them in the
editor with **Project > Project settings** (Ctrl+Shift+P, Cmd+Shift+P on macOS), then **Apply**. Closing the window
without **Apply** discards the changes.

Settings can also be edited in the file while the editor is closed, or with the `project_settings_set` tool of the
[[mcp-server]]. Missing settings keep their default value.

::: warning
Some settings are read when the engine starts, before the project is opened. They are ignored by the editor and by run
mode, which includes the Play button. They are marked [late](#late) below.
:::

## App {#app}

| Setting           | Default                     | Description                                                                          |
| ----------------- | --------------------------- | ------------------------------------------------------------------------------------ |
| `project_name`    | `New Atmo Project`          | Title of the window, and [[projects#user-paths\|`user://`]] folder of exported games |
| `default_scene`   | Empty                       | [[projects#default-scene\|First scene]] of the game                                  |
| `icon_path`       | `project://assets/atmo.png` | Not used yet                                                                         |
| `engine_version`  | Version of the engine       | Only in the file. Set when the project is created.                                   |
| `project_version` |                             | Only in the file. Used in the name of `.pck` files.                                  |

## Boot {#boot}

`background_color`, `show_splash_icon`, `full_size_splash_icon` and `splash_icon_path` are not used yet.

## Window {#window}

| Setting            | Default     | Description                                                                     |
| ------------------ | ----------- | ------------------------------------------------------------------------------- |
| `size`             | 1280 by 720 | Initial size of the window ([late](#late)), and size that `stretch_mode` scales |
| `background_color` | `#191919`   | Color behind the scene ([late](#late) for the game window)                      |
| `stretch_mode`     | `disabled`  | `disabled`, `fit` or `integer`: see [[screen-sizes#stretch-mode]]               |
| `letterbox_color`  | Black       | Color of the bars of `fit` and `integer` ([late](#late))                        |
| `headless`         | No          | Not used. Start the engine with `--headless` instead.                           |

The scene editor outlines `size` in the viewport.

## Rendering {#rendering}

| Setting          | Default  | Description                                                |
| ---------------- | -------- | ---------------------------------------------------------- |
| `texture_filter` | `linear` | `linear` or `nearest`: see [[screen-sizes#texture-filter]] |

The settings window does not show this section: edit the file or use the [[mcp-server]].

## Engine {#engine}

| Setting              | Default      | Description                                                 |
| -------------------- | ------------ | ----------------------------------------------------------- |
| `gravity`            | (0, 9.81, 0) | Gravity in m/s². `z` is not used.                           |
| `physics_frame_rate` | 60           | Physics steps per second ([late](#late))                    |
| `pixels_per_meter`   | 50           | Conversion between pixels and meters: see [[physics#units]] |

## Debug {#debug}

| Setting                      | Default | Description                                    |
| ---------------------------- | ------- | ---------------------------------------------- |
| `draw_physics_debug`         | Yes     | Draws the collision shapes in the scene editor |
| `draw_physics_debug_in_game` | No      | Draws them in the game ([late](#late))         |

## Singletons {#singletons}

`scenes` is not used yet.

## MCP {#mcp}

| Setting   | Default | Description                                     |
| --------- | ------- | ----------------------------------------------- |
| `enabled` | No      | Starts the [[mcp-server]] when the editor opens |
| `port`    | 27760   | Port of the server                              |

## Addons {#addons}

One checkbox per addon of the project: see [[addons]]. The change applies the next time the project is opened.

## Late settings {#late}

When a project is opened with `--project`, the window and the game world are created before its settings are read, with
the default values of the settings. This affects the settings marked above. Exported games read their settings first.
