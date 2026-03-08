# Contributing to VneTemplate

Thank you for your interest in contributing. This document explains how to build, test, and format the project so your contributions fit the repository standards.

## Building

- **Prerequisites:** CMake 3.19+, C++20 compiler. See [README.md](README.md#prerequisites).
- **Dependencies:** Initialize submodules from the project root:
  ```bash
  git submodule update --init --recursive
  ```
- **Configure and build:**
  ```bash
  cmake -B build/shared -DCMAKE_BUILD_TYPE=Debug -DVNE_TEMPLATE_TESTS=ON
  cmake --build build/shared
  ```
- **Tests and examples:** `-DVNE_TEMPLATE_DEV=ON`, or set `-DVNE_TEMPLATE_TESTS=ON` and `-DVNE_TEMPLATE_EXAMPLES=ON` separately.
- **Platform scripts:** See [scripts/README.md](scripts/README.md) for `build_linux.sh`, `build_macos.sh`, `build_windows.sh`, `build_windows.py`, and `build_windows.ps1`.

## Testing

- Run tests:
  ```bash
  ctest --test-dir build/shared -C Debug --output-on-failure
  ```
- Or use the script test action, e.g. `./scripts/build_macos.sh -a test`.

## Code style and formatting

- **Formatting** is enforced in CI. Before pushing, run the project formatter so CI passes:
  ```bash
  ./scripts/format.sh
  ```
  Or use the same command as CI: format `src`, `include`, `examples`, and `tests` with the project `.clang-format` (see [.github/workflows/ci.yml](.github/workflows/ci.yml)).
- **Style and naming** are defined in [CODING_GUIDELINES.md](CODING_GUIDELINES.md). Formatting is enforced by [.clang-format](.clang-format) and static analysis by [.clang-tidy](.clang-tidy). Please follow these so reviews can focus on design and logic.

## Pull requests

- Ensure the project builds and tests pass (locally or via CI).
- Keep changes focused. CHANGELOG.md is updated manually when cutting a release (see [README.md](README.md#releases)).
- If you add or change behavior, add or update tests as appropriate.

## Code review

- Reviews follow the repository [Copilot PR review instructions](.github/copilot-instructions.md) and [CODING_GUIDELINES.md](CODING_GUIDELINES.md). Addressing feedback on correctness, API design, and test quality will speed up merging.
