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
https://docs.atmo-engine.com/set-up

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
- Comment must follows the Doxygen convention (        



        LINK TO DOXYGEN



)

### Naming Conventions



    EXPLAIN CLANG TIDY RULES



- Use descriptive and meaningful names
- Avoid abbreviations unless they are widely understood
- Be consistent with existing naming patterns in the codebase

Examples:
good: renderFrame()
bad: rf()

good: playerPosition
bad: pp                         Remove this                         

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

On VS Code you can install the extension (
    link to extension                           
    ) to apply this format each time you save your code
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
