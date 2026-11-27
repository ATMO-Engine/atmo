# Contributing to Atmo Engine

First off, thank you for considering contributing to Atmo Engine
This document provides guidelines to help you contribute effectively.


## Table of Contents

- [Reporting Issues](#reporting-issues)
- [Feature Requests](#feature-requests)
- [Development Setup](#development-setup)
- [Project Structure](#project-structure)
- [Coding Guidelines](#coding-guidelines)
- [Formatting](#formatting)
- [Testing](#testing)
- [Documentation](#documentation)
- [Releasing](#releasing)
- [Translations](#translations)
- [Pull Requests](#pull-requests)
- [Commit Messages](#commit-messages)
- [Code of Conduct](#code-of-conduct)


## Reporting Issues

If you encounter a bug, please open an issue and include:

- A clear and descriptive title
- Steps to reproduce the issue
- Expected behavior vs actual behavior
- Logs, screenshots, or error messages if applicable

### System Information

If possible include the following details:
- OS: (e.g. Windows 11, Ubuntu 22.04)
- CPU (with architecture):
- GPU:
- GPU Driver Version:
- RAM:
- Compiler: (e.g. MSVC, Clang, GCC + version)
- Build configuration: (Debug/Release)
- Commit/Branch: (if not using latest main)

Before creating a new issue, please check if it already exists.


## Feature Requests

We welcome suggestions!

When proposing a feature, please include:

- The problem you're trying to solve
- A clear description of the proposed solution
- Possible alternatives (if any)


## Development Setup

For full setup instructions, see the documentation:
https://docs.atmo-engine.com/latest/getting-started/building-from-source

### Prerequisites

Make sure the following tools are installed:

- Git
- xmake (build system)

#### Compiler

- **Windows**: Microsoft Visual C++ (MSVC)
- **macOS**: clang++
- **Linux**: g++

Make sure your compiler is installed and available in your environment.

### Quick Start

Fork the main repository and clone then build your fork
After your code is finished create a Pull request from your fork into Atmo Engine

```
git clone <your-repository-url>
cd <project-folder>
xmake
```

The first build downloads and builds the third-party libraries through xmake's package manager. There are no submodules to initialize.
If you change a dependency version in `xmake.lua`, commit the updated `xmake-requires.lock` with it. See [Third-party dependencies](https://docs.atmo-engine.com/latest/getting-started/building-from-source#dependencies) in the documentation.

Build Configurations

You can build the project in different modes:

```
xmake f -m debug
xmake
```

```
xmake f -m release
xmake
```

### Troubleshooting

- If `xmake` cannot find your compiler, verify your toolchain installation
- Ensure your compiler is available in your system `PATH`
- Check the documentation for platform-specific setup steps

### Notes

- The documentation contains more detailed setup instructions and platform-specific guides
- If you encounter issues, consider opening an issue with your system information


## Coding Guidelines

To maintain consistency and code quality across the project, please follow these guidelines:


### General Principles

- Write clear, readable, and maintainable code
- Prefer simplicity over clever or complex solutions
- Keep functions small and focused on a single responsibility
- Avoid premature optimization unless necessary
- Comment only when needed (explain *why*, not *what*)
- Comment must follows the Doxygen convention (https://www.doxygen.nl/manual/docblocks.html#cppblock)

### Naming Conventions

This project uses `clang-tidy` to enforce naming consistency across the codebase.

#### Identifier Rules

##### Classes, Structs, and Enums

Use `CamelCase`:

```cpp
class RenderPipeline {};
struct WindowConfig {};
enum RenderMode {};
```

##### Methods

Use `camelBack` for methods and functions:

```cpp
void renderFrame();
void initializeRenderer();
```

##### Member Variables

Member variables must use `lower_case` with prefixes depending on visibility:

```cpp
private:
    int m_frame_count;

protected:
    int p_render_width;
```

Prefixes:
- `m_` → private members
- `p_` → protected members

#### General Guidelines

- Use descriptive and meaningful names
- Avoid abbreviations unless they are widely understood
- Be consistent with existing naming patterns in the codebase

Examples:

Good:
```cpp
renderFrame();
playerPosition;
```

Bad:
```cpp
rf();
pp;
```

#### Additional clang-tidy Rules

The project also enables several modernization and safety checks, including:

- `modernize-use-nullptr`
- `modernize-use-default-member-init`
- `modernize-use-bool-literals`
- `modernize-deprecated-headers`
- `cppcoreguidelines-pro-type-member-init`

Contributors are expected to follow these rules and resolve related warnings when modifying code.

### Code Style

- Follow the existing style of the project
- Keep a consistent indentation and formatting
- Group related code logically
- Avoid large, monolithic files or functions

### C++ Specific Guidelines

- Prefer modern C++ features when appropriate (e.g. smart pointers, RAII)
- Avoid raw pointers unless necessary
- Use `const` correctness wherever possible
- Minimize global state
- Be explicit with ownership and lifetimes

### Error Handling

- Handle errors gracefully
- Avoid silent failures
- Provide meaningful error messages when possible

### Dependencies and Architecture

- Do not introduce new dependencies without discussion
- Keep modules loosely coupled
- Respect the existing architecture and layering

### Consistency

- If you modify existing code, follow its style
- Do not mix different styles in the same file
- When in doubt, match the surrounding code


## Formatting

This project uses `clang-format` to enforce a consistent coding style.

### Rules

- All code must be formatted using the provided `.clang-format` file
- Do not manually format code in ways that conflict with the formatter
- Run the formatter before committing any changes

### How to Format

On VS Code you can install the extension (https://marketplace.visualstudio.com/items?itemName=xaver.clang-format) to apply this format each time you save your code
You can format files manually using:

For every file in src:
```bash
find ./src -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.c" \) -exec clang-format -i {} +
```

```powershell
Get-ChildItem -Path .\src -Recurse -Include *.cpp,*.h,*.hpp,*.c | ForEach-Object { clang-format -i $_.FullName }
```

Or each for each file individually:
```bash
clang-format -i <path/to/file>
```

```powershell
clang-format -i <.\path\to\file>
```


## Testing

We use unit tests to ensure the stability and correctness of the engine.

### Running Tests

All tests can be executed using `xmake`:

```bash
xmake test
```
This will:
- Build the test targets
- Run all registered unit tests
- Output results directly in the terminal

### Writing Tests

- Place new tests in the appropriate test directory (e.g. `tests/`)
- Follow existing test structure and naming conventions
- Ensure tests are deterministic and do not depend on external state

### Guidelines

- Every new feature should include corresponding tests
- Bug fixes should include regression tests when possible
- Keep tests fast and focused (unit-level, not integration-heavy unless necessary)

### Continuous Integration

Make sure:
- All tests pass before submitting a pull request
- No new warnings or errors are introduced


## Documentation

The guides and manual of the [documentation site](https://docs.atmo-engine.com) live in [`docs/`](docs/), and are published from each engine version.

- When a pull request changes behavior described in `docs/en/`, update those pages in the same pull request
- Public APIs are documented with Doxygen comments in the source; the site generates the C++ API reference from them
- The `docs` workflow checks the pages' formatting and fails on broken references, such as a link to a class that was renamed

See [`docs/README.md`](docs/README.md) for how to write and preview pages.


## Releasing

Releases are built by the `release` workflow (`.github/workflows/release.yml`) when a version tag is pushed. It builds
the editor (`atmo`) and the export runtime (`atmo-export`) in release mode on Windows (x64), macOS (arm64) and Linux
(x64), and attaches `atmo_<version>_<platform>.zip`, `atmo-export_<version>_<platform>.zip` and `SHA256SUMS.txt` to a
draft GitHub release. Nothing is signed.

1. On `main`, set `ATMO_VERSION` in `src/project/project_settings.hpp` to the new version and merge it.
2. Tag that commit with `v` followed by the same version, and push the tag:

   ```bash
   git tag -a v1.1.0 -m "Atmo 1.1.0" <commit on main>
   git push origin v1.1.0
   ```

3. When the workflow is done, review the draft release on GitHub and publish it.

The workflow fails before building if the tag is not `v<ATMO_VERSION>` or if the tagged commit is not on `main`.
Versions with a prerelease part (`v1.1.0-beta`) are marked as prereleases. Running the workflow again for the same tag
replaces the files of the draft. Only tags on commits that contain the workflow start it, so tags made before it was
added never do.

The release builds for macOS use `--target_minver=13.0`, so that they start on macOS 13 and later; their packages are
cached separately from the other builds.

### Icons

The icons live in `packaging/` (not in `assets/`, which is packed into the editor):

| File                                                | Used by                                                     |
| --------------------------------------------------- | ----------------------------------------------------------- |
| `packaging/icons/atmo.png`                          | Source of the macOS icon (1024×1024, macOS icon grid)       |
| `packaging/icons/atmo-256.png`                      | Editor window icon (embedded in `atmo`), Linux `atmo.png`   |
| `packaging/windows/atmo.ico`, `atmo.rc`             | Icon of `atmo.exe`                                          |
| `packaging/macos/atmo.icns`, `Info.plist.in`        | `atmo_<version>_macos.app`                                  |
| `packaging/linux/atmo.desktop`                      | Linux menu entry                                            |

They are made from `assets/Atmo-exports/Plapipou/PNG/Dark/Atmo-platy-dark.png`, centered on a white rounded square
(824 px wide, 185 px corner radius, in a 1024 px transparent image). `atmo-256.png` and `atmo.ico` are that square
cropped to its edges, `atmo.ico` holds the sizes 16, 24, 32, 48, 64, 128 and 256. To rebuild `atmo.icns` on macOS
from an updated `atmo.png`:

```bash
mkdir atmo.iconset
for size in 16 32 128 256 512; do
  sips -z $size $size packaging/icons/atmo.png --out atmo.iconset/icon_${size}x${size}.png
  sips -z $((size * 2)) $((size * 2)) packaging/icons/atmo.png --out atmo.iconset/icon_${size}x${size}@2x.png
done
iconutil -c icns atmo.iconset -o packaging/macos/atmo.icns
```

`atmo-export` has no icon, so exported games are not shown with the ATMO icon.

## Translations

Atmo Engine is translated by the community using [Crowdin](https://crowdin.com/project/atmo-engine).

- Source strings live in `translation/en-US/*.json`
- Translations are synchronized into `translation/<locale>/` (see `crowdin.yml`)
- Only edit the `en-US` source files directly in the repository; other languages should be translated through Crowdin

### Adding new strings

- Add new keys to the `en-US` JSON files only
- Use clear, descriptive keys and avoid concatenating translated strings in code

### Credits

All translators are credited in the [Translators](AUTHORS.md#translators) section of `AUTHORS.md`.
Once your translations have been approved, open a pull request adding your name under your language
(or mention it in your translation pull request) so that we can add you.


## Pull Requests

To submit a contribution, please follow this workflow:

1. Fork the repository
2. Create a new branch from `main`
3. Make your changes
4. Commit your work
5. Push your branch
6. Open a Pull Request

### Branch Naming

Branch names should clearly describe what is being worked on.

Examples:
png-loader-fix
ecs-refactor

Use lowercase and dash, and keep names concise but descriptive.

### Pull Request Title

Pull request titles must follow this format:
[verb] action done

Examples:
[fix] crashes on png load
[add] tracy profiler integration

### Guidelines

- Keep pull requests small and focused
- Describe clearly **what** was changed and **why**
- Link related issues if applicable
- Ensure the project builds successfully
- Ensure all tests pass
- Format your code before submitting (see Formatting section)

### Review Process

- Be responsive to feedback and requested changes
- Discussions should remain constructive and focused
- Changes may be requested before merging

### Notes

- Pull requests that do not follow naming or formatting conventions may be rejected
- If your contribution is large or impacts architecture, consider opening an issue first to discuss it


## Commit Messages

Commit messages must follow a clear and consistent format:
[verb] action done

Examples:
[fix] entity scripting system
[edit] variables names

### Rules

- Use lowercase
- Keep messages short and descriptive
- Focus on **what** the change does

### Guidelines

- Keep commits small and focused
- Make sure the project builds before each commit

### Notes

- Pull request titles must follow the same format
- Commits that do not follow this convention may be requested to be rewritten


## Code of Conduct

To ensure a welcoming and respectful community, all contributors are expected to follow these principles:

### General Behavior

- Be respectful and considerate of others
- Provide constructive feedback
- Be open to discussion and different perspectives
- Focus on the project and avoid personal attacks

### Unacceptable Behavior

- Harassment, discrimination, or offensive language
- Personal attacks or insults
- Any form of disruptive or toxic behavior

### Enforcement

Project maintainers reserve the right to:

- Request changes in behavior
- Moderate or remove inappropriate content
- Reject contributions that do not follow these guidelines

### Notes

- This Code of Conduct applies to all project spaces (issues, pull requests, discussions)
