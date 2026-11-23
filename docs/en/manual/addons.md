---
id: addons
title: Addons
order: 15
description: Create addons that share assets or extend the engine with a native C++ library.
---

# Addons

An addon is a folder of a project that holds assets, or a native library that extends the engine with C++ code.

## Create an addon {#create}

```sh
atmo --project /path/to/my-game --new-addon "My Addon" assets
atmo --project /path/to/my-game --new-addon "My Addon" library
```

The addon is created in the `addons` folder of the project, enabled, and added to the
[[project-settings#addons|project settings]]. Characters other than letters, digits, `_` and `-` are replaced by `_` in
the name of its folder:

```text
addons/My_Addon/
├── addon.json
├── assets/              # assets addon
├── src/My_Addon.cpp     # library addon
└── xmake.lua            # library addon
```

## The manifest {#manifest}

`addon.json` describes the addon. Its fields are those of [[AddonHeader]]:

| Field           | Description                                                                         |
| --------------- | ----------------------------------------------------------------------------------- |
| `name`          | Displayed name                                                                      |
| `internal_name` | Name of the folder and of the library                                               |
| `version`       | Version of the addon                                                                |
| `atmo_version`  | Version of the engine the addon is made for. It must be exactly that of the engine. |
| `license_title` | License                                                                             |
| `authors`       | List of authors                                                                     |
| `website`       | Website                                                                             |
| `source_link`   | Where the source code is                                                            |
| `has_library`   | Whether the addon has a native library to load                                      |

## Enable an addon {#enable}

Addons are in the `addons` section of `.atmo/project_settings.json`, with their path and whether they are enabled:

```json
"addons": { "addons": { "project://addons/My_Addon": true } }
```

The project settings window has a checkbox for each of them. To install an addon made elsewhere, copy its folder into
`addons` and add it there. Addons are loaded when the project is opened or the game starts.

## Use the assets {#assets}

The files of an addon are files of the project. They are available even when the addon is disabled:

```luau
this:setMaterial("project://addons/My_Addon/assets/glow.material")
```

## Write a library {#library}

A library has one entry point, declared with [[ATMO_REGISTER_ADDON]] and called when the addon is loaded:

```cpp
#include "addon/addon.hpp"
#include <spdlog/spdlog.h>

using namespace atmo::addon;

ATMO_REGISTER_ADDON()
{
    spdlog::info("Hello world from My Addon!");
}
```

`api`, an [[AtmoAPI]], gives access to the engine:

| Member                            | Description                               |
| --------------------------------- | ----------------------------------------- |
| [[AtmoAPI::engine]]               | The engine, with its world                |
| [[AtmoAPI::input_manager]]        | Named inputs                              |
| [[AtmoAPI::arg_manager]]          | Command-line arguments                    |
| [[AtmoAPI::entity_registry]]      | Entity types: see [[adding-entities]]     |
| [[AtmoAPI::subresource_registry]] | Subresource types, such as physics shapes |
| [[AtmoAPI::meta_registry]]        | Component types                           |
| [[AtmoAPI::rendering_server]]     | Rendering                                 |

Build it with the same compiler and the same version of the source code as the engine. `ATMO_DIR` is the folder of the
engine:

```sh
cd addons/My_Addon
ATMO_DIR=/path/to/atmo xmake build My_Addon
```

The library must be next to `addon.json`, under the internal name of the addon: `My_Addon.so` on Linux, `My_Addon.dll` on
Windows and `My_Addon.dylib` on macOS.

::: warning
A library runs with all the rights of the engine. Only install addons you trust.
:::

## Exported games {#export}

Exported games do not load addons at the moment: libraries are not called, and the log says `addon.json not found`. The
assets of addons are packed and can be used.

## Errors {#errors}

An addon that cannot be loaded is skipped, and the reason is logged:

| Message                                             | Cause                                                 |
| --------------------------------------------------- | ----------------------------------------------------- |
| `addon.json not found`                              | The path in the settings is wrong                     |
| `ATMO version mismatch`                             | `atmo_version` is not the version of the engine       |
| `dlopen failed`, `LoadLibrary failed`               | The library is missing, not built, or misses a symbol |
| `Symbol 'atmo_register_addon_entrypoint' not found` | The library has no [[ATMO_REGISTER_ADDON]]            |
