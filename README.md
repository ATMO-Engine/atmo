# ATMO Engine

A lightweight C++ game engine with Luau scripting and ECS architecture.


# Requirements

## Windows
- C++23 compiler (MSVC)
- xmake

## MacOs
- C++23 compiler (clang++)
- xmake
- Xcode, only when building the Vulkan platform (MoltenVK is built with `xcodebuild`)

## Linux
- c++23 compiler (g++)
- xmake

Install dependencies:
https://xmake.io/guide/quick-start.html


# Build
```
git clone https://github.com/ATMO-Engine/atmo.git
cd atmo
xmake
```

The first `xmake` run downloads and builds every third-party library, including the Slang shader compiler, so it takes a while. Later builds reuse them.

## Rendering platforms

The engine picks the best rendering platform for the machine it runs on:

| OS      | Tried in order        |
|---------|-----------------------|
| macOS   | Metal, Vulkan, Null   |
| Windows | Vulkan, Null          |
| Linux   | Vulkan, Null          |

A platform is skipped when it is not built or when the GPU lacks what it needs. A platform that only finds a software rasterizer (such as lavapipe) is used only if no other platform has a real GPU. Null draws nothing; it is what `--headless` uses. If no GPU platform can start outside headless mode, the engine shows an error dialog and quits instead of opening an empty window.

There is no D3D12 platform: Vulkan is the Windows backend. D3D12 is planned for later, to be developed on a Windows machine.

`--rendering-platform=metal|vulkan|null` asks for a specific platform; if it is unavailable, the engine falls back to the automatic choice.

```
xmake run atmo --rendering-platform=vulkan --gpu-validation
```

- Metal needs macOS 13 or later and a GPU in the Metal 3 family with tier 2 argument buffers: every Apple silicon Mac and the recent AMD and Intel GPUs. Older Macs fall back to Vulkan when it is built, otherwise to Null. On macOS 15 and later the platform keeps its resources resident with a residency set; on 13 and 14 it declares them on every render pass instead. `ATMO_METAL_RESIDENCY_SETS=0` forces the macOS 13 path, for testing.
- The Metal platform is built by default on macOS (`xmake f --rendering_metal=n` turns it off). It uses [metal-cpp](https://developer.apple.com/metal/cpp/), so it is plain C++; no Objective-C is involved. Shaders are embedded as Metal source and compiled by the system when the engine starts; compiled pipelines are cached in `~/Library/Application Support/Atmo/Atmo/pipeline-cache/`, which can be deleted at any time. Vulkan keeps its pipeline cache in the same folder (`vulkan-pipelines.bin`), and discards it when the GPU or driver changes.
- With `--gpu-validation`, Metal turns on its API and shader validation (`MTL_DEBUG_LAYER`, `MTL_SHADER_VALIDATION`). Captures can be taken with Xcode's GPU debugger.
- Vulkan needs a Vulkan 1.3 driver with dynamic rendering, synchronization2 and descriptor indexing. On Windows the driver from the GPU vendor (NVIDIA, AMD, Intel, Qualcomm) provides it; the generic Microsoft display driver used by some virtual machines and remote sessions has no Vulkan.
- The Vulkan platform is built by default on Windows and Linux. On macOS it is opt-in, because it needs MoltenVK (a long first build that requires Xcode) and Metal is the macOS backend: run `xmake f --rendering_vulkan=y` once, then `--rendering-platform=vulkan` to use it. The build then copies `libMoltenVK.dylib` next to the binary. A Vulkan loader installed on the system (for example from the Vulkan SDK) is used first, which is what makes `--gpu-validation` work.
- Do not commit the `xmake-requires.lock` changes that `--rendering_vulkan=y` causes on macOS: the lock file matches the default configuration of each platform.
- On Vulkan, `--gpu-validation` needs the Khronos validation layer (`VK_LAYER_KHRONOS_validation`) to be installed.
- `xmake f --rendering_vulkan=n` builds without the Vulkan platform on Windows and Linux.
- Rendering runs on its own thread; `--render-thread=off` runs it on the main thread for debugging.

## Screen sizes and HiDPI

Positions, sizes and scales in a scene are in game pixels. On a HiDPI (Retina) screen they are drawn with more screen pixels, so a scene looks the same size on a 1x and a 2x screen.

How a scene reacts to the window size is set in the project settings:

- `window.stretch_mode`:
  - `"disabled"` (default): a larger window shows more of the world.
  - `"fit"`: the game is laid out for `window.size` and scales to fill the window, keeping its aspect ratio; the rest of the window shows bars in `window.letterbox_color`.
  - `"integer"`: like `fit`, but only scales by whole numbers of screen pixels, for crisp pixel art.
- `rendering.texture_filter`: `"linear"` (default, smooth) or `"nearest"` (sharp pixels, for pixel art).

## Shaders and materials

A game shader is a [Slang](https://shader-slang.org) file in the project that defines one struct implementing `atmo::ICanvasMaterial`. The engine keeps the vertex stage, batching and texture access; the shader only decides the color of each pixel. The struct's fields are the material's parameters.

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

- `atmo::CanvasFragment` gives `uv`, `color` (the command color times the sprite's modulate), `position` (canvas units), `screen_position` (target pixels), `custom` (the sprite's `custom` value) and `sample(uv)` for the sprite's texture. `atmo::time()` is the time in seconds since the engine started.
- Parameters can be `int`, `uint`, `float`, `float2`, `float3`, `float4` and `atmo::Texture`. `[Color]` on a `float4` shows a color picker, `[Range(min, max)]` documents the expected range. `bool` is not allowed (Metal cannot load it from a buffer): use a `uint` set to 0 or 1.
- Other `.slang` files next to the shader can be imported (`import my_helpers;`).
- A material is a `.material` file that names the shader and stores the parameter values. Open it with the material editor, which lists the shader's parameters and previews the result. Many sprites can share one material and still draw in one batch.
- `Sprite2d` has `material_path`, `modulate` (tints the sprite, with or without a material) and `custom` (four numbers the material reads as `input.custom`, so one material can still vary per sprite).
- Post-processing: a struct implementing `atmo::IViewportMaterial` receives an `atmo::ViewportFragment` (`uv`, `screen_position`, `size`, `sample(uv)` of the rendered world). List such materials in a `Camera2d`'s `post_process`; they run in order on the world, and the UI is drawn on top afterwards.
- Scripts change parameters with `sprite:getMaterial():set("strength", 0.02)`. A material is shared, so the change applies to every sprite using it.
- In the editor, saving a shader recompiles it; sprites keep their materials and parameter values. A shader that fails to compile keeps its previous version and logs Slang's errors.
- Exporting compiles every material shader for Vulkan (SPIR-V) and Metal (MSL) and packs the results, because exported games do not include the shader compiler.

## Dependencies

Third-party libraries are installed by xmake's package manager from [xmake-repo](https://github.com/xmake-io/xmake-repo).
There are no git submodules to clone.

- Versions are declared with `add_requires` in `xmake.lua`.
- The exact versions in use are recorded in `xmake-requires.lock`. Commit it whenever a version changes.
- Recipes for versions or options xmake-repo does not provide yet live in `deps/packages/`.
- Built packages are cached in `~/.xmake/packages` and shared between checkouts.

To change a dependency version, edit its `add_requires` line, then run `xmake f -y` and commit the updated lock file.

On Linux, SDL3 needs the system development packages for X11, Wayland and audio, and Vulkan needs `libvulkan1` plus a driver.
See the "Install SDL3 build dependencies" step in `.github/workflows/build.yml` for the list.


# Documentation

Full documentation available here:
https://docs.atmo-engine.com

# Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting changes.


# Authors

Atmo Engine is developed by a community of voluntary contributors.
See [AUTHORS.md](AUTHORS.md) for the list of founders, main contributors and translators.

Want to help translate Atmo Engine? See the [Translations](CONTRIBUTING.md#translations) section of the contributing guide.


# License

This project is licensed under the MIT License.

It includes third-party software with separate licenses.
See THIRD_PARTY_LICENSES.md for details.
