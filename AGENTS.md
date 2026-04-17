# AGENTS.md

Guidance for coding agents working in this repository.

## Project Summary

- Project: `drea`
- Language: C++17
- Build system: CMake
- Main library target: `drea` (defined in `core/`)
- Test executable: `drea-test` (Catch2)
- Optional examples: built under `examples/`

This repository provides a framework for building CLI apps/services with commands, options, and configuration sources.

## Repository Layout

- `core/`: library implementation and integration code.
- `include/`: public headers.
- `tests/`: Catch2-based tests.
- `examples/`: sample CLI applications.
- `cmake/`: shared CMake modules/macros.

## Build And Test

Typical local workflow:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Build without examples when iterating on core/tests:

```bash
cmake -S . -B build -DBUILD_EXAMPLES=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

## Dependencies

- Dependency discovery is done through `find_package(...)`.
- `core/CMakeLists.txt` supports using `VCPKG_ROOT` to set a vcpkg toolchain automatically when `CMAKE_TOOLCHAIN_FILE` is not set.
- Tests require `Catch2`.

If dependency resolution fails, prefer fixing CMake/vcpkg configuration rather than hardcoding paths.

## Coding Conventions

- Keep the codebase on C++17.
- Follow existing formatting/style in touched files:
  - Brace and spacing style as currently used.
  - Naming patterns and namespace layout already present in `core/` and `include/`.
- Make focused changes; avoid broad refactors unless explicitly requested.
- Preserve existing compile definitions and feature toggles (for example `BUILD_REST_USE`, `ENABLE_JSON`, `ENABLE_TOML`).

## Testing Expectations For Changes

- For behavior changes in `core/`, add or update tests in `tests/` whenever practical.
- At minimum, run build and tests locally before concluding work:
  - `cmake --build build`
  - `ctest --test-dir build --output-on-failure`

## Agent Guardrails

- Do not remove or rename public headers/APIs unless explicitly requested.
- Do not introduce new third-party dependencies without explicit approval.
- Keep changes scoped to the user request.
