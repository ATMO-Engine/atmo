---
id: texture-editor
title: Texture editor
order: 13
description: Draw images with layers and frames, import sprite sheets, and export images.
---

# Texture editor

The [[TextureEditor|texture editor]] draws images made of layers and animation frames. Open it with **+** in the tab bar.

::: danger
`.atx` images, the format of the editor, currently [cannot be opened again](#known-issues). Export your work to `.png`.
:::

## Draw {#draw}

A new image is 64 by 64 pixels. Change its size with **Width** and **Height**, up to 65535.

| Action             | Effect                       |
| ------------------ | ---------------------------- |
| Left mouse button  | Draws or erases              |
| Right mouse button | Moves the image, by dragging |
| Scroll             | Zooms, from 0.5 to 10 times  |

- **Draw** paints with the color of the color picker, blended according to its opacity. **Erase** makes pixels
  transparent.
- **Size** is the diameter of the brush, in pixels.
- **Preview** shows the [[projects#default-scene|default scene]] of the project. It does nothing if none is set.

There is no undo.

## Layers {#layers}

| Button        | Effect                                                                            |
| ------------- | --------------------------------------------------------------------------------- |
| `+`           | Adds a layer                                                                      |
| `-`           | Deletes the layer                                                                 |
| `^` and `v`   | Move the selected layer up or down                                                |
| Hide and Show | Change whether the layer is shown. The button above the list changes all of them. |

The first layer of the list is on top.

## Frames {#frames}

Every layer has the same number of frames, up to 255. In the timeline, press a cell to select its layer and frame.

| Button                    | Effect                                                |
| ------------------------- | ----------------------------------------------------- |
| **Add Frame**             | Adds a frame at the end                               |
| **Delete Frame**          | Deletes the current frame, unless it is the only one  |
| **Play** and **Pause**    | Play the animation. Drawing is locked while it plays. |
| **Previous** and **Next** | Change the current frame                              |
| **FPS**                   | Speed of the animation, from 1 to 240                 |

## Import an image {#import}

Open a `.png`, `.jpg` or `.bmp` file from the file explorer of the editor, or with **Open** while a texture editor tab is
active. The import window asks for:

- **Frame count**, from 1 to 255;
- **Frame width** and **Frame height**, which are the size of the whole image by default.

Frames are read from left to right, then from top to bottom. The result has a single layer. To import a plain image, keep
the default values.

## Export {#export}

**Export** asks for a name and a format, and writes the file next to the open one.

| Format                 | Contents                                                                               |
| ---------------------- | -------------------------------------------------------------------------------------- |
| `.png`, `.bmp`, `.jpg` | The selected layers, blended. Frames are side by side in a single row: a sprite sheet. |
| `.atx`                 | The image with its layers and frames                                                   |

A sprite sheet can be imported again with the same frame size. `.jpg` has no transparency.

**Save** writes the open file. If it is an image, it becomes a sprite sheet of all the layers, and its layers and frames
are lost.

## Use the image in a game {#use}

Export it to `.png` and write its path in `texture_path`: see [[page:entities#sprite2d]]. Sprites cannot use `.atx` files, and
do not animate sprite sheets.

## File explorer {#file-explorer}

The file explorer lists the files of the project. Select a file or a folder, then:

| Button         | Effect                                           |
| -------------- | ------------------------------------------------ |
| **Add File**   | Creates an empty file with the name in the field |
| **Add Folder** | Creates a folder                                 |
| **Rename**     | Renames the selected file or folder              |
| **Delete**     | Asks for confirmation, then deletes it           |

## The .atx format {#atx}

All numbers are little-endian.

| Data                             | Type                                          |
| -------------------------------- | --------------------------------------------- |
| `ATMOIMG` and a zero byte        | 8 characters                                  |
| Version, 1                       | 32-bit unsigned integer                       |
| Number of layers                 | 32-bit unsigned integer                       |
| Number of frames                 | 32-bit unsigned integer                       |
| Width                            | 32-bit unsigned integer                       |
| Height                           | 32-bit unsigned integer                       |
| Then, for each layer:            |                                               |
| Length of the name, and the name | 32-bit unsigned integer, then characters      |
| Visible                          | 1 byte                                        |
| Pixels of each frame, row by row | Four 32-bit decimals: red, green, blue, alpha |

## Known issues {#known-issues}

- Opening an `.atx` file fails: the log says `file format not valid`, and the tab shows a blank image. Saving that tab
  replaces the file with the blank image.
- The **Dropdown** button is a test and has no effect.
