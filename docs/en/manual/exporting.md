---
id: exporting
title: Exporting a game
order: 14
description: Turn a project into a standalone executable, and check that it contains everything it needs.
---

# Exporting a game

An exported game is a single executable: a copy of `atmo-export`, the engine without the editor, followed by the files of
the project. Players need neither the engine nor the project.

## Export {#export}

1. Build the export runtime, in the folder of the engine:

    ```sh
    xmake f -m release
    xmake build atmo-export
    ```

2. Set `default_scene` in the [[project-settings#app|project settings]]: the game starts with it.
3. Export the project, with the editor binary:

    ```sh
    atmo --project /path/to/my-game --export /path/to/atmo-export /path/to/my-game-executable
    ```

There is no export button in the editor. On Windows, give the executable a name that ends in `.exe`.

The game runs on the system `atmo-export` was built for. To export for another system, use an `atmo-export` built there.

## What is packed {#packed}

Every file of the project folder is packed, including the scripts, the project settings and files that the game does not
use. Keep anything private out of the project folder.

[[shaders-and-materials#editing-and-exporting|Material shaders]] are compiled during the export, and their errors are
logged.

To list the files of an exported game, or print one of them:

```sh
atmo --headless --read /path/to/my-game-executable
atmo --headless --read /path/to/my-game-executable main.ascn
```

## Before exporting {#checklist}

- Use [[projects#project-paths|project:// paths]] everywhere. Other paths only work on your computer.
- Copy the engine files that the game uses into the project, at the same place. The engine's files are found in the editor
  but **not in exported games**:

    | File                             | Used by                                                  |
    | -------------------------------- | -------------------------------------------------------- |
    | `assets/fonts/Nunito/Nunito.ttf` | Labels and text fields, unless their font is changed     |
    | `assets/atmo.png`                | Nothing, unless the game shows it. New projects have it. |

    They are in the `assets` folder of the engine's source code. Copy the license of the fonts too.

- Check the upper and lower case of the paths, which matters on every system once exported.
- Run the exported game once from a terminal, and read its log for `File not found in packed file system`.

## Differences with run mode {#differences}

| In run mode                                          | In an exported game                                        |
| ---------------------------------------------------- | ---------------------------------------------------------- |
| [[project-settings#late\|Late settings]] are ignored | Every setting is read before the game starts               |
| The project is read from the disk                    | The files are inside the executable, and read-only         |
| `user://` is the folder of the engine                | `user://` is a [[projects#user-paths\|folder of the game]] |
| The [[mcp-server]] can be used                       | There is no MCP server                                     |
| Addons are loaded                                    | Addons are [not loaded](#known-issues)                     |

Check in the exported game that the window size and the physics behave as expected, since they can differ from what the
Play button shows.

## Command-line options {#options}

Exported games accept `--headless`, `--rendering-platform`, `--render-thread`, `--gpu-validation` and `--lang`: see
[[building-from-source#command-line]].

## Known issues {#known-issues}

- Addons are not loaded: the log says `addon.json not found in addon directory`, even though the file is packed. Their
  assets can still be used by their path.
- The default font is missing unless the project contains it.
- `Locale 'en-US' not available` is logged when the game starts, and has no effect on games.
