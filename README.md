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
| Windows | D3D12, Vulkan, Null   |
| Linux   | Vulkan, Null          |

A platform is skipped when it is not built or when the GPU lacks what it needs. A platform that only finds a software rasterizer (such as lavapipe) is used only if no other platform has a real GPU. Null draws nothing; it is what `--headless` uses. The Metal and D3D12 platforms do not exist yet, so for now every OS lands on Vulkan when it is built.

`--rendering-platform=metal|d3d12|vulkan|null` asks for a specific platform; if it is unavailable, the engine falls back to the automatic choice.

```
xmake run atmo --rendering-platform=vulkan --gpu-validation
```

The game, the UI and the editor are all drawn by the RenderingServer. A macOS build without the Vulkan platform therefore opens an empty window until the Metal platform lands; build with `xmake f --rendering_vulkan=y` (below) to work on macOS in the meantime.

- Vulkan needs a Vulkan 1.3 driver with dynamic rendering, synchronization2 and descriptor indexing.
- The Vulkan platform is built by default on Windows and Linux. On macOS it is opt-in, because it needs MoltenVK (a long first build that requires Xcode) and Metal will be the macOS backend: run `xmake f --rendering_vulkan=y` once. The build then copies `libMoltenVK.dylib` next to the binary. A Vulkan loader installed on the system (for example from the Vulkan SDK) is used first, which is what makes `--gpu-validation` work.
- Do not commit the `xmake-requires.lock` changes that `--rendering_vulkan=y` causes on macOS: the lock file matches the default configuration of each platform.
- `--gpu-validation` needs the Khronos validation layer (`VK_LAYER_KHRONOS_validation`) to be installed.
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
