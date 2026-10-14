# ATMO Engine

A lightweight C++ game engine with Luau scripting and ECS architecture.


# Requirements

## Windows
- C++23 compiler (MSVC)
- xmake

## MacOs
- C++23 compiler (clang++)
- xmake

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

The first `xmake` run downloads and builds every third-party library, so it takes a while. Later builds reuse them.

## Dependencies

Third-party libraries are installed by xmake's package manager from [xmake-repo](https://github.com/xmake-io/xmake-repo).
There are no git submodules to clone.

- Versions are declared with `add_requires` in `xmake.lua`.
- The exact versions in use are recorded in `xmake-requires.lock`. Commit it whenever a version changes.
- Recipes for versions xmake-repo does not provide yet live in `deps/packages/`.
- Built packages are cached in `~/.xmake/packages` and shared between checkouts.

To change a dependency version, edit its `add_requires` line, then run `xmake f -y` and commit the updated lock file.

On Linux, SDL3 needs the system development packages for X11, Wayland and audio.
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
