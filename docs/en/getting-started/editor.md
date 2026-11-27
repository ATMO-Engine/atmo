---
id: editor
title: Using the editor
order: 4
description: Create a project, build a scene, run it, and find your way around the editor.
---

# Using the editor

## Create or open a project {#project}

Start the engine without options to get the project list:

- To create a project, type its name, then press **Create new project**.
- To open one, press its path in the list.

The list shows the projects of the [[projects#location|projects folder]]. Open any other one with
`atmo --project /path/to/project`.

## The window {#window}

| Part           | Contents                                                                                |
| -------------- | --------------------------------------------------------------------------------------- |
| Menus          | File, Edit, Project and Debug. On macOS, they are in the system menu bar.               |
| Tab bar        | One tab per open editor, and **+** to open another one. Press a tab's icon to close it. |
| Editor         | The active tab                                                                          |
| Bottom toolbar | Play                                                                                    |

Closing a tab does not ask to save it.

## Open and save files {#files}

Each kind of file has its editor:

| Editor                              | Extension   | Documentation              |
| ----------------------------------- | ----------- | -------------------------- |
| [[SceneEditor\|Scene editor]]       | `.ascn`     | [This page](#scene-editor) |
| [[MaterialEditor\|Material editor]] | `.material` | [[shaders-and-materials]]  |
| [[TextureEditor\|Texture editor]]   | `.atx`      | [[texture-editor]]         |

- **+** opens an empty editor.
- **Open** asks for a file. If the active tab has the right editor, the file replaces its contents; otherwise a new tab
  is opened.
- **Save** saves the active tab, and asks for a file the first time. Scenes get the `.ascn` extension when it is missing.

Scripts and shaders are written in the text editor of your choice.

::: danger
When a file cannot be loaded in an editor, the tab stays open and empty, and saving it replaces the file. Do not save
after an error in the log. This concerns in particular prefabs, which [[prefabs#change|cannot be opened as scenes]], and
[[texture-editor#known-issues|.atx images]].
:::

## Shortcuts {#shortcuts}

| Shortcut     | Action                                 |
| ------------ | -------------------------------------- |
| Ctrl+S       | Save                                   |
| Ctrl+Shift+S | Save as                                |
| Ctrl+O       | Open                                   |
| Ctrl+Z       | Undo, in a text field                  |
| Ctrl+Shift+Z | Redo, in a text field                  |
| Ctrl+Shift+P | [[project-settings\|Project settings]] |
| Ctrl+Shift+D | Add demo entities to the scene         |
| Ctrl+Q       | Quit                                   |

On macOS, use Cmd instead of Ctrl. Undo and redo only apply to the text being typed: changes to a scene cannot be undone.

## The scene editor {#scene-editor}

| Part        | Contents                                         |
| ----------- | ------------------------------------------------ |
| Left panel  | The entity tree                                  |
| Center      | The viewport                                     |
| Right panel | The inspector: components of the selected entity |

### Viewport {#viewport}

| Action                               | Effect |
| ------------------------------------ | ------ |
| Scroll                               | Move   |
| Ctrl+scroll (Cmd on macOS), or pinch | Zoom   |

The red and green lines are the axes of the world, and the white rectangle is the size of the game window. `x` grows to
the right and `y` downwards. Entities cannot be selected or moved with the mouse in the viewport: use the tree and the
inspector. UI entities are only visible in the game.

### Entity tree {#tree}

The **+** button of the panel adds an entity to the scene. Each row has four buttons:

| Button | Effect                                                    |
| ------ | --------------------------------------------------------- |
| Plus   | Adds a child to the entity                                |
| Folder | [[prefabs#load-editor\|Loads a prefab]] as a child        |
| Disk   | [[prefabs#create-editor\|Saves the entity as a prefab]]   |
| Cross  | Deletes the entity and its children, without confirmation |

Press a row to select the entity. The types are described in [[page:entities]].

### Inspector {#inspector}

The inspector shows the fields of each component of the selected entity. **Add Script** adds a [[page:entities#script|script]] to
it.
Components cannot be added or removed otherwise: they come with the type of the entity.

Paths are typed by hand: write [[projects#project-paths|project:// paths]]. Some fields, such as the layout of UI
entities, show their name without any way to change them: edit the scene file instead.

## Run the game {#play}

1. Set `default_scene` in the [[project-settings#app|project settings]], for example to `project://main.ascn`.
2. Save the open files: the game reads them from the disk.
3. Press Play. The game runs in its own window, and the button stops it.

## A first game {#first-game}

1. Create a project.
2. Press **+** in the tab bar and choose the scene editor.
3. Add a `Sprite2d` with the **+** button of the tree, select it, and write `project://assets/atmo.png` in
   `texture_path`.
4. Set its `position` to (400, 300).
5. Save the scene as `main.ascn` in the project.
6. In the project settings, write `project://main.ascn` in `default_scene`, then press **Apply**.
7. Press Play.

Then make it move with a script: see [[scripting#first-script]].
