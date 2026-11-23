---
id: projects
title: Projects and files
order: 2
description: Where projects are stored on your computer and how to reach their files with project:// paths.
---

# Projects and files

## Where projects are stored {#location}

Projects created from the project list, the window that opens when the engine starts without `--project`, are stored
in the `projects` folder of the engine's user data directory:

| Platform | Location                                                      |
| -------- | ------------------------------------------------------------- |
| Windows  | `%APPDATA%\atmo\userdata\projects\<name>`                     |
| macOS    | `~/Library/Application Support/atmo/userdata/projects/<name>` |
| Linux    | `~/.local/share/atmo/userdata/projects/<name>`                |

`<name>` is the name typed when creating the project. The project list shows every project found there, with its full
path.

A project can also live anywhere else on the disk. It is then not in the list, and is opened by its path:

```sh
atmo --project /path/to/my-game         # open in the editor
atmo --project /path/to/my-game --run   # run the game without the editor
```

To move a project, move its whole folder: paths written with [`project://`](#project-paths) keep working.

## What a project contains {#layout}

```text
my-game/
├── .atmo/
│   └── project_settings.json
└── assets/
    └── atmo.png
```

| Path                          | Contents                                                                |
| ----------------------------- | ----------------------------------------------------------------------- |
| `.atmo/project_settings.json` | The project settings. A folder is a project when it contains this file. |
| `assets/atmo.png`             | The default project icon.                                               |

These are the only files the engine creates. Everything else is up to you: scenes, scripts, images and materials can be
in any folder of the project. The examples in this documentation use `assets/script/` for scripts and `assets/prefabs/`
for prefabs.

## Reaching files with project:// {#project-paths}

`project://` stands for the project's root folder. What follows is the path of the file inside the project, with `/` as
the separator on every platform. In a project stored in `~/.local/share/atmo/userdata/projects/my-game`:

| Path                                  | File                                                                      |
| ------------------------------------- | ------------------------------------------------------------------------- |
| `project://main.ascn`                 | `~/.local/share/atmo/userdata/projects/my-game/main.ascn`                 |
| `project://assets/script/player.luau` | `~/.local/share/atmo/userdata/projects/my-game/assets/script/player.luau` |

Write two slashes: `project:/main.ascn` is not recognized, and is looked up as an ordinary path on the disk.

`project://` paths work wherever the engine loads a file:

- in the editor, in path fields such as `script_path`, `texture_path` or `material_path`;
- in the project settings, for example in `default_scene`;
- in scripts, in the functions that take a path: see [[luau-api#prefab]], [[luau-api#sprite2d]] and
  [[luau-api#material]].

Scripts have no function to read, write or list files themselves.

### Why not another path {#other-paths}

| Path                        | What happens                                                                         |
| --------------------------- | ------------------------------------------------------------------------------------ |
| `project://assets/hero.png` | Works in the editor, in run mode, in exported games and on other computers.          |
| `/home/me/my-game/hero.png` | Only works on this computer.                                                         |
| `assets/hero.png`           | Relative to the directory the engine was started from, which may not be the project. |

### Where project:// looks {#lookup}

In the editor and in run mode, the engine looks for the file:

1. in the project folder;
2. in the files packed inside the engine itself, such as its fonts and icons.

This is why `project://assets/fonts/Nunito/Nunito.ttf` works in a project that has no fonts. It also means that a file
of the project hides an engine file with the same path.

An exported game contains the whole project folder, and `project://` reads from inside the executable. These files are
read-only. The engine's own files are not looked up there: copy the ones your game uses into the project before
exporting.

### File not found {#not-found}

`File not found in packed file system: "assets/hero.png"` means that the file was found neither in the project nor in
the engine. Check the spelling, including upper and lower case, which matters on Linux.

## user:// paths {#user-paths}

`user://` stands for the user data directory, outside of the project:

| Platform | Editor and run mode                           | Exported game                                  |
| -------- | --------------------------------------------- | ---------------------------------------------- |
| Windows  | `%APPDATA%\atmo\userdata`                     | `%APPDATA%\<project_name>`                     |
| macOS    | `~/Library/Application Support/atmo/userdata` | `~/Library/Application Support/<project_name>` |
| Linux    | `~/.local/share/atmo/userdata`                | `~/.local/share/<project_name>`                |

`<project_name>` is the name in the project settings. Game assets belong in the project, with `project://`: what is in
`user://` is not exported.

## The scene the game starts with {#default-scene}

Set `default_scene` in the project settings to the `.ascn` file of the first scene, for example `project://main.ascn`.
The Play button, `--run` and exported games load it. `--run` can also be given another scene:

```sh
atmo --project /path/to/my-game --run project://levels/level_2.ascn
```

The scripts attached to the entities of that scene are what runs first: continue with [[scripting]].
