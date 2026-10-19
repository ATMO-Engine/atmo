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

`--rendering-platform=metal|vulkan|null` picks the rendering platform. Without it, the engine tries Metal first on macOS and uses Null elsewhere; a platform that is unavailable falls back to that default. The Metal platform does not exist yet, so the default still ends on Null.

The rendering rework is in progress. Sprites, cameras and physics debug drawing now go through the new 2D canvas renderer, while the UI and the editor still use SDL_Renderer until the next phase. In the meantime:

- On the default platform (Null), the UI and editor work, but scenes draw nothing.
- On Vulkan, scenes are drawn by the new renderer, but there is no UI yet.

```
xmake run atmo --rendering-platform=vulkan
xmake run atmo --rendering-platform=vulkan --gpu-validation
```

- Vulkan needs a Vulkan 1.3 driver with dynamic rendering, synchronization2 and descriptor indexing. If none is found, the engine falls back to the default platform.
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
