---
id: ui
title: User interface
order: 12
description: UI entities, how they are laid out, and what games can do with them today.
---

# User interface

UI entities draw over the game, in window coordinates: the camera does not move them.

::: warning Limits
The UI system is used by the editor and is not finished for games:

- Scripts cannot read or change UI entities, or know that a button was pressed.
- Most layout options cannot be changed in the inspector, and some are not stored in scene files.
- Scenes with some of the widgets [cannot be opened again](#known-issues).

:::

## Show a label {#first-label}

1. In the scene editor, add a `UIRect` to the scene, and a `UILabel` inside it.
2. Select the label and write its `text`.
3. Press Play: the scene editor does not show UI entities.

## UI entities {#types}

| Type                 | Draws                                   | Can be saved in a scene |
| -------------------- | --------------------------------------- | ----------------------- |
| `UI`                 | Nothing: groups and places its children | Yes                     |
| `UIRect`             | A rectangle with a border and corners   | Yes                     |
| `UILabel`            | Text                                    | Yes                     |
| `UIImage`            | An image                                | Yes                     |
| `UIButton`           | A `UIRect` that reacts to the mouse     | Yes                     |
| `UIInput`            | A text field                            | Yes                     |
| `UITextInput`        | A text field for a string               | Yes                     |
| `UINumberInput`      | A text field for a number               | Yes                     |
| `UICheckBox`         | A checkbox                              | [No](#known-issues)     |
| `UISlider`           | A slider                                | [No](#known-issues)     |
| `UiDropdownMenu`     | A menu that opens under the mouse       | [No](#known-issues)     |
| `UIFoldableTreeItem` | A section that folds                    | [No](#known-issues)     |

The other UI types in the list belong to the editor.

A UI entity must be the child of another UI entity, or of an entity that is not UI, in which case it is placed from the
top-left corner of the window. Several UI entities directly in the scene are placed side by side.

## UI {#ui-component}

Every UI entity has this component:

| Field      | Default | Description                                                                             |
| ---------- | ------- | --------------------------------------------------------------------------------------- |
| `visible`  | Yes     | Hides the entity and its children. Ignored on UI entities placed directly in the scene. |
| `modulate` | White   | Tint. For a label, the color of the text.                                               |

## Layout {#layout}

`Layout` sets the size of an entity and the place of its children, which are in a row. The inspector shows its fields but
cannot change them. Write them in the scene file:

```json
"Layout": {
    "width": { "type": 2, "size": { "min": 200, "max": 200 } },
    "height": { "type": 0, "size": { "min": 0, "max": 0 } },
    "padding": { "left": 8, "right": 8, "top": 8, "bottom": 8 },
    "gap": 4,
    "floating": { "enabled": false, "offset": { "x": 0, "y": 0 } }
}
```

| Field      | Description                                                                                         |
| ---------- | --------------------------------------------------------------------------------------------------- |
| `width`    | [Size](#sizes) on the horizontal axis                                                               |
| `height`   | [Size](#sizes) on the vertical axis                                                                 |
| `padding`  | Space between the edges and the children, in pixels                                                 |
| `gap`      | Space between two children, in pixels                                                               |
| `floating` | When `enabled`, places the entity at `offset` from the top-left corner of the window, over the rest |

### Sizes {#sizes}

| `type` | Name    | Size of the entity           | `size`                               |
| ------ | ------- | ---------------------------- | ------------------------------------ |
| 0      | Fit     | That of its contents         | `min` and `max`, where 0 is no limit |
| 1      | Grow    | The space left by the others | `min` and `max`, where 0 is no limit |
| 2      | Fixed   | `min` pixels                 | The same `min` and `max`             |
| 3      | Percent | A part of its parent         | A number from 0 to 1                 |

The direction (horizontal), the alignment of the children, the aspect ratio, the scrolling and the depth order are not
stored in scene files.

## Rect {#rect}

| Field           | Default  | Description                                                                               |
| --------------- | -------- | ----------------------------------------------------------------------------------------- |
| `color`         | White    | Color of the rectangle                                                                    |
| `corner_radius` | 0        | `top_left`, `top_right`, `bottom_left`, `bottom_right`. In the scene file only.           |
| `border`        | 0, white | `left`, `right`, `top`, `bottom`, `between_children` and `color`. In the scene file only. |

## Text {#text}

| Field            | Default | Description                                           |
| ---------------- | ------- | ----------------------------------------------------- |
| `text`           | Empty   | The text                                              |
| `font_path`      | Nunito  | A `.ttf` or `.otf` file                               |
| `font_bold`      | No      | Bold                                                  |
| `font_italic`    | No      | Italic                                                |
| `font_size`      | 24      | In the scene file only, like the fields below         |
| `letter_spacing` | 0       | Space between letters                                 |
| `line_height`    | 0       | Height of a line. 0 uses that of the font.            |
| `wrap_mode`      | 0       | 0 wraps between words, 1 only at line breaks, 2 never |
| `text_alignment` | 0       | 0 left, 1 center, 2 right                             |

The default font is an engine file. Copy a font into the project before [[exporting|exporting the game]].

## Image {#image}

| Field          | Default | Description           |
| -------------- | ------- | --------------------- |
| `texture_path` | Empty   | Image to draw         |
| `rotation`     | 0       | Rotation of the image |

## Button {#button}

| Field    | Default | Description                                                                        |
| -------- | ------- | ---------------------------------------------------------------------------------- |
| `toggle` | No      | The button stays pressed until it is pressed again                                 |
| `group`  | 0       | Among the toggle buttons with the same group, one is pressed at a time. 0 is none. |

C++ code can connect to the signals of a button: see [[signals-and-events#ui-signals]].

## Text fields {#input}

`UIInput` and its variants have these fields: `text`, `placeholder`, `input_type` (0 text, 1 integer, 2 decimal, in the
scene file only), `multiline`, `read_only`, `password`, `max_length` and `font_size` (in the scene file only, 0 for no
limit), `font_path`, `text_color` and `placeholder_color`.

## Known issues {#known-issues}

- A scene or prefab saved with a `UICheckBox`, a `UISlider`, a `UiDropdownMenu` or a `UIFoldableTreeItem` makes the
  engine crash when it is loaded. These widgets create their own children, which are saved with the scene and then created
  twice. If a scene is already affected, remove the children of the widget from the file with a text editor.
- The default font is missing from exported games, unless the project contains it: see [[exporting#checklist]].
