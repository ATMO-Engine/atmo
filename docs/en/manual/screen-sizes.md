---
id: screen-sizes
title: Screen sizes and HiDPI
order: 3
description: How scenes are scaled to the window and to HiDPI screens, and the project settings that control it.
---

# Screen sizes and HiDPI

Positions, sizes and scales in a scene are in game pixels. On a HiDPI (Retina) screen they are drawn with more
screen pixels, so a scene looks the same size on a 1x and a 2x screen.

## Stretch mode {#stretch-mode}

How a scene reacts to the window size is set by `window.stretch_mode` in the [[ProjectSettings|project settings]]:

| Value                  | Behavior                                                                                                                                                       |
| ---------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `"disabled"` (default) | A larger window shows more of the world.                                                                                                                       |
| `"fit"`                | The game is laid out for `window.size` and scales to fill the window, keeping its aspect ratio. The rest of the window shows bars in `window.letterbox_color`. |
| `"integer"`            | Like `fit`, but only scales by whole numbers of screen pixels, for crisp pixel art.                                                                            |

## Texture filtering {#texture-filter}

`rendering.texture_filter` sets how textures are sampled when scaled:

| Value                | Behavior                    |
| -------------------- | --------------------------- |
| `"linear"` (default) | Smooth                      |
| `"nearest"`          | Sharp pixels, for pixel art |

For pixel art, combine `"nearest"` with the `"integer"` stretch mode.
