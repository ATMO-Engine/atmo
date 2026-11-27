---
id: installing
title: Installing a release
order: 1
description: Download the editor and the export runtime for Windows, macOS or Linux.
---

# Installing a release

Each version of ATMO is published on the [releases page](https://github.com/ATMO-Engine/atmo/releases) as two zip
files per system: the editor, and the export runtime used to [[exporting|export games]]. To build the engine
yourself instead, see [[building-from-source]].

## Downloads {#downloads}

`<version>` is the engine version, such as `1.0.0`.

| System  | Editor                       | Export runtime                      |
| ------- | ---------------------------- | ----------------------------------- |
| Windows | `atmo_<version>_windows.zip` | `atmo-export_<version>_windows.zip` |
| macOS   | `atmo_<version>_macos.zip`   | `atmo-export_<version>_macos.zip`   |
| Linux   | `atmo_<version>_linux.zip`   | `atmo-export_<version>_linux.zip`   |

Each zip contains one program, named like the zip (`atmo_1.0.0_windows.exe`, `atmo_1.0.0_macos.app`,
`atmo_1.0.0_linux`, `atmo-export_1.0.0_linux`…), with the engine's license and the licenses of the libraries it uses.
`SHA256SUMS.txt` lists the checksum of every zip.

Use the export runtime of the same version as the editor.

## Requirements {#requirements}

| System  | Needs                                                                                     |
| ------- | ----------------------------------------------------------------------------------------- |
| Windows | 64-bit Windows (x64), a graphics driver with Vulkan 1.3                                   |
| macOS   | macOS 13 or later on Apple silicon (M1 or later). Intel Macs are not supported            |
| Linux   | x86-64, glibc 2.39 or later (Ubuntu 24.04, Debian 13, Fedora 40 or later), Vulkan 1.3 GPU |

## First launch {#first-launch}

The programs are not signed, so the system warns about them the first time.

### Windows {#windows}

Extract the zip and start `atmo_<version>_windows.exe`. If SmartScreen says that it protected your PC, click
**More info**, then **Run anyway**.

### macOS {#macos}

Extract the zip and move `atmo_<version>_macos.app` to the Applications folder. macOS blocks programs downloaded
from the internet that are not signed, and may say that the app "is damaged". Remove the download flag once, in a
terminal:

```sh
xattr -dr com.apple.quarantine /Applications/atmo_<version>_macos.app
```

Do the same for the export runtime before exporting with it:

```sh
xattr -d com.apple.quarantine atmo-export_<version>_macos
```

### Linux {#linux}

Extract the zip and start `./atmo_<version>_linux`. The zip keeps the executable permission. If it was lost, run
`chmod +x atmo_<version>_linux` first.

To start the editor from the application menu with its icon, install the program, the icon and the menu entry for
your user:

```sh
mkdir -p ~/.local/bin ~/.local/share/applications ~/.local/share/icons/hicolor/256x256/apps
cp atmo_<version>_linux ~/.local/bin/atmo
cp atmo.png ~/.local/share/icons/hicolor/256x256/apps/atmo.png
cp atmo.desktop ~/.local/share/applications/atmo.desktop
```

`~/.local/bin` must be on your `PATH`. The menu entry starts the program named `atmo`.

## Icons {#icons}

The editor has the ATMO icon: in the `.exe` on Windows, in the app bundle on macOS, and on its window on Windows and
Linux. Exported games do not have an icon yet.
