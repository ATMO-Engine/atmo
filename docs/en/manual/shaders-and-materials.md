---
id: shaders-and-materials
title: Shaders and materials
order: 4
description: Write Slang shaders for sprites and post-processing, and set their parameters with materials.
---

# Shaders and materials

A game shader is a [Slang](https://shader-slang.org) file in the project that defines one struct implementing
`atmo::ICanvasMaterial`. The engine keeps the vertex stage, batching and texture access; the shader only decides
the color of each pixel. The struct's fields are the material's parameters.

```slang
import atmo;

public struct Wobble : atmo::ICanvasMaterial
{
    [Range(0, 0.1)] float strength;
    [Color] float4 tint;
    float speed;
    atmo::Texture noise;

    float4 fragment(atmo::CanvasFragment input)
    {
        float wave = sin(input.uv.y * 30.0 + atmo::time() * speed) * strength;
        float grain = noise.sample(input.uv * 4.0).r;
        return input.sample(input.uv + float2(wave, 0.0)) * input.color * tint * grain;
    }
};
```

Other `.slang` files next to the shader can be imported (`import my_helpers;`).

## Fragment input {#fragment-input}

`atmo::CanvasFragment` gives:

| Member            | Contents                                               |
| ----------------- | ------------------------------------------------------ |
| `uv`              | Texture coordinates                                    |
| `color`           | The draw command's color times the sprite's `modulate` |
| `position`        | Position in canvas units                               |
| `screen_position` | Position in target pixels                              |
| `custom`          | The sprite's `custom` value                            |
| `sample(uv)`      | Samples the sprite's texture                           |

`atmo::time()` is the time in seconds since the engine started.

## Parameters {#parameters}

Parameters can be `int`, `uint`, `float`, `float2`, `float3`, `float4` and `atmo::Texture`.

- `[Color]` on a `float4` shows a color picker.
- `[Range(min, max)]` documents the expected range.
- `bool` is not allowed, because Metal cannot load it from a buffer: use a `uint` set to 0 or 1.

## Materials {#materials}

A material is a `.material` file that names the shader and stores the parameter values. Open it with the
[[MaterialEditor|material editor]], which lists the shader's parameters and previews the result. Many sprites can
share one material and still draw in one batch.

[[entities::Sprite2d]] has three fields for materials:

- `material_path` — the material to draw with.
- `modulate` — tints the sprite, with or without a material.
- `custom` — four numbers the material reads as `input.custom`, so one material can still vary per sprite.

Scripts change parameters through the sprite:

```luau
sprite:getMaterial():set("strength", 0.02)
```

A material is shared, so the change applies to every sprite using it.

## Post-processing {#post-processing}

A struct implementing `atmo::IViewportMaterial` receives an `atmo::ViewportFragment`: `uv`, `screen_position`,
`size`, and `sample(uv)` of the rendered world. List such materials in a [[Camera2d]]'s `post_process`; they run in
order on the world, and the UI is drawn on top afterwards.

## Editing and exporting {#editing-and-exporting}

- In the editor, saving a shader recompiles it; sprites keep their materials and parameter values. A shader that
  fails to compile keeps its previous version and logs Slang's errors.
- Exporting compiles every material shader for Vulkan (SPIR-V) and Metal (MSL) and packs the results, because
  exported games do not include the shader compiler.
