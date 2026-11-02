---
id: building-from-source
title: Building from source
order: 1
description: Install the toolchain and build the engine, its tests and the export runtime.
---

# Building from source

## Requirements {#requirements}

ATMO builds with [xmake](https://xmake.io) and a C++23 compiler:

| Platform | Compiler | Also needed                                                                    |
| -------- | -------- | ------------------------------------------------------------------------------ |
| Windows  | MSVC     |                                                                                |
| macOS    | clang++  | Xcode, only to build the [[rendering#vulkan\|Vulkan platform]] (MoltenVK)      |
| Linux    | g++      | Development packages for X11, Wayland and audio ([see below](#linux-packages)) |

See the [xmake quick start](https://xmake.io/guide/quick-start.html) to install xmake.

### Linux packages {#linux-packages}

SDL3 is built from source and needs the system development packages for X11, Wayland and audio. Vulkan needs
`libvulkan1` and a driver. On Debian and Ubuntu:

```sh
sudo apt-get install -y --no-install-recommends \
    build-essential git make pkg-config cmake ninja-build \
    libasound2-dev libpulse-dev libaudio-dev libfribidi-dev libjack-dev libsndio-dev \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev \
    libxss-dev libxtst-dev libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev \
    libgles2-mesa-dev libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev \
    libpipewire-0.3-dev libwayland-dev libxinerama-dev \
    libvulkan1 mesa-vulkan-drivers vulkan-validationlayers
```

The engine's CI installs the same list, in the "Install SDL3 build dependencies" step of
`.github/workflows/build.yml`.

## Clone and build {#build}

```sh
git clone https://github.com/ATMO-Engine/atmo.git
cd atmo
xmake
```

`xmake` builds the `atmo` target (engine and editor) in debug mode. Then start it with:

```sh
xmake run atmo
```

The first `xmake` run downloads and builds every third-party library, including the Slang shader compiler, so it
takes a while. Later builds reuse them. There are no git submodules to initialize.

## Other targets and modes {#targets}

| Command                   | Result                                                      |
| ------------------------- | ----------------------------------------------------------- |
| `xmake -m release`        | Optimized build (`-O3`)                                     |
| `xmake build atmo-test`   | Catch2 unit test binary                                     |
| `xmake test -v`           | Run the unit tests                                          |
| `xmake build atmo-export` | Export-only runtime, without the editor                     |
| `xmake clean --mode=soft` | Remove build artifacts                                      |
| `xmake clean --mode=full` | Remove build artifacts and generated files                  |
| `xmake clean --mode=all`  | Also uninstall the project's packages (rebuilt on next run) |

Debug builds enable AddressSanitizer and UndefinedBehaviorSanitizer on platforms other than Windows.
`xmake f -m debug --tsan=y` uses ThreadSanitizer instead.

## Build options {#options}

Options are set once with `xmake f` and kept for later builds.

| Option                    | Default                  | Effect                                                     |
| ------------------------- | ------------------------ | ---------------------------------------------------------- |
| `--rendering_metal=y\|n`  | on for macOS             | Builds the [[rendering#metal\|Metal rendering platform]]   |
| `--rendering_vulkan=y\|n` | on for Windows and Linux | Builds the [[rendering#vulkan\|Vulkan rendering platform]] |
| `--tsan=y\|n`             | off                      | ThreadSanitizer in debug builds, instead of ASan/UBSan     |

## Command-line options {#command-line}

Pass options to the engine after the target name, for example `xmake run atmo --headless`. `--help` lists them
all.

| Option                                     | Effect                                                                     |
| ------------------------------------------ | -------------------------------------------------------------------------- |
| `--headless`                               | Starts without a window, using the Null rendering platform                 |
| `--rendering-platform=metal\|vulkan\|null` | Asks for a [[rendering#selection\|rendering platform]]                     |
| `--gpu-validation`                         | Turns on the rendering platform's [[rendering#gpu-validation\|validation]] |
| `--render-thread=on\|off`                  | `off` renders on the main thread, which keeps stack traces readable        |
| `--lang=<language>`                        | Overrides the detected user interface language                             |

## Third-party dependencies {#dependencies}

Third-party libraries are installed by xmake's package manager from
[xmake-repo](https://github.com/xmake-io/xmake-repo).

- Versions are declared with `add_requires` in `xmake.lua`.
- The exact versions in use are recorded in `xmake-requires.lock`. Commit it whenever a version changes.
- Recipes for versions or options xmake-repo does not provide yet live in `deps/packages/`.
- Built packages are cached in `~/.xmake/packages` and shared between checkouts.

To change a dependency version, edit its `add_requires` line, then run `xmake f -y` and commit the updated lock
file.

::: warning
The lock file matches the default configuration of each platform. Do not commit the `xmake-requires.lock` changes
that `--rendering_vulkan=y` causes on macOS.
:::

Once the engine builds, continue with the [[architecture]] overview.
