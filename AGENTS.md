# AGENTS.md

## Purpose

- This repository is a Windows-first C++ OpenGL engine starter.
- The project name for this repository is `Engine`.
- Do not refer to this project as `CASCADE`, `Project BONK`, or `BONK` in code, docs, tests, or generated content.
- Treat it as a long-lived foundation, not a tutorial throwaway.
- Optimize for maintainability, readable architecture, and low-risk iteration.
- Preserve the split between app lifecycle, rendering, and shader management.

## Rule Files

- No `.cursor/rules/` files are present today.
- No `.cursorrules` file is present today.
- No `.github/copilot-instructions.md` file is present today.
- If any of those files are added later, read them before editing code.

## Repository Layout

- `src/main.cpp` only boots the application and handles fatal errors.
- `src/Application.*` owns GLFW startup, the window, input, buffer swaps, and event polling.
- `src/core/Renderer.*` owns frame setup, projection/view state, and draw submission.
- `src/core/Shader.*` owns file loading, shader compilation, link validation, and uniforms.
- `src/geometry/` owns CPU-side geometry data, procedural 2D generators, and geometry operations.
- `src/graphics/` contains thin OpenGL wrappers and the `Mesh` class.
- `src/primitives/` contains reusable geometry sources such as `Triangle`, `Quad`, `Plane`, `Cube`, `Pyramid`, `Sphere`, `Cylinder`, `Cone`, `Capsule`, and `Torus`.
- `src/math/Types.*` contains foundational vector, color, and matrix helpers.
- `src/math/Transform.*` contains transform composition helpers.
- `assets/shaders/` contains runtime GLSL assets and must stay file-based.
- `tests/` contains lightweight smoke tests registered through CTest.
- `external/glfw/` is vendored GLFW source.
- `external/glad/` contains generated GLAD loader files.
- `.vscode/` contains shared VS Code workspace configuration.

## Preferred Workflow

- Use CMake presets instead of ad hoc build directories.
- Default local workflow: Ninja + Debug preset.
- Keep Visual Studio support working; do not break the `vs2022-x64` preset.
- Keep shader assets loadable from both source tree and runtime output directory.
- Do not move third-party dependency layout unless CMake and docs are updated too.

## Configure Commands

- Debug Ninja: `cmake --preset debug-ninja`
- Release Ninja: `cmake --preset release-ninja`
- Visual Studio 2022 x64: `cmake --preset vs2022-x64`
- Legacy manual Ninja configure still works: `cmake -S . -B build -G Ninja`
- Legacy manual VS configure still works: `cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64`

## Build Commands

- Debug Ninja build: `cmake --build --preset build-debug-ninja`
- Release Ninja build: `cmake --build --preset build-release-ninja`
- Visual Studio Debug build: `cmake --build --preset build-vs2022-debug`
- Visual Studio Release build: `cmake --build --preset build-vs2022-release`
- Manual Ninja build: `cmake --build build`
- Manual VS Debug build: `cmake --build build-vs --config Debug`

## Run Commands

- Ninja runtime path: `out/build/debug-ninja/bin/EngineStarter.exe`
- Visual Studio runtime path: `out/build/vs2022-x64/bin/Debug/EngineStarter.exe`
- Shader files are copied post-build; if the window opens but nothing draws, verify `assets/shaders/` exists next to the executable.

## Test Commands

- Run all Debug Ninja tests: `ctest --preset test-debug-ninja --output-on-failure`
- Run all Release Ninja tests: `ctest --preset test-release-ninja --output-on-failure`
- Run all VS Debug tests: `ctest --preset test-vs2022-debug --output-on-failure`
- Run a single test by exact or regex name: `ctest --preset test-debug-ninja -R shader_assets_exist --output-on-failure`
- List available tests from a build tree: `ctest --test-dir out/build/debug-ninja -N`
- Current tests are smoke tests for repo layout and required assets.

## Lint And Format

- There is no standalone linter target yet.
- Compiler warnings are treated as the first line of linting.
- GCC/Clang builds use `-Wall -Wextra -Wpedantic`.
- MSVC builds use `/W4 /permissive-`.
- Format with the repository `.clang-format` file.
- If `clang-format` is installed, use: `clang-format -i src/*.cpp src/*.h tests/*.cpp`
- In VS Code, format-on-save is enabled for C and C++.

## VS Code Notes

- Open the repository root folder directly in VS Code.
- Install the recommended extensions from `.vscode/extensions.json`.
- CMake Tools is configured to use presets automatically.
- The default configure/build/test preset is the Debug Ninja path.
- Use `Tasks: Run Task` for configure, build, all-tests, or single-test runs.
- Use `F5` with the matching launch profile for GDB or MSVC debugging.

## Architecture Expectations

- Keep `main.cpp` tiny.
- Keep high-level object wiring in `main.cpp`, not low-level GL setup.
- Keep windowing and platform interactions inside `Application`.
- Keep frame setup and draw submission inside `Renderer`.
- Keep shader file I/O and program creation inside `Shader`.
- Keep CPU-side shape generation and topology operations inside `geometry/`.
- Keep raw buffer and vertex-array ownership inside `graphics/` wrappers and `Mesh`.
- Keep reusable primitive data inside `primitives/`, not in `main.cpp`.
- Add new systems as focused classes or subsystems, not by bloating existing files.
- Favor seams that will scale to textures, camera, ECS, batching, and particles.

## Includes And Dependencies

- Use project headers first, then third-party headers, then standard library headers.
- Keep include lists minimal and intentional.
- Prefer forward declarations in headers when ownership and size rules allow.
- Include `<glad/glad.h>` before `<GLFW/glfw3.h>` in translation units that need both.
- Do not include OpenGL headers from public headers unless required by the interface.
- Avoid leaking GLFW or GL details through unrelated interfaces.

## Formatting Style

- Follow `.clang-format`.
- Use 4-space indentation.
- Use Allman braces.
- Keep lines readable; prefer under 100 columns.
- Use one declaration per line for clarity.
- Avoid dense one-liners except for trivial returns.

## Naming Conventions

- Types use `PascalCase`.
- Member functions use `camelCase`.
- Data members use `m_` prefix.
- Namespaces use lowercase.
- Constants with internal linkage may use `constexpr` variables in anonymous namespaces.
- Test names should be descriptive and snake_case for easy `ctest -R` matching.

## Types And Ownership

- Prefer standard library types over custom utility wrappers until a real need exists.
- Use `std::filesystem::path` for paths.
- Use `std::unique_ptr` for exclusive ownership.
- Delete copy and move operations on owning systems unless semantics are deliberate.
- Prefer `int` for sizes passed directly to GLFW APIs that expect `int`.
- Cast explicitly when converting to OpenGL-specific integer or size types.

## Error Handling

- Fail early on initialization problems.
- Throw `std::runtime_error` with specific context for unrecoverable setup failures.
- Include file paths in shader and filesystem error messages when relevant.
- Validate shader compile and program link status every time.
- Do not silently swallow GL, filesystem, or window creation failures.
- Use `main.cpp` as the final catch-and-report boundary.

## OpenGL Rules

- Target OpenGL 3.3 Core Profile.
- Do not introduce deprecated immediate-mode APIs.
- Use VAOs for vertex input state.
- Keep shader sources in external files, never inline strings.
- Load GLAD only after the GLFW context is current.
- Keep render setup and per-frame rendering separated.
- Prefer GPU-driven animation through uniforms and shaders over CPU-side vertex mutation.
- Be explicit about buffer ownership and deletion.

## Shader Rules

- Store shaders under `assets/shaders/`.
- Keep shader filenames stable unless all call sites and docs are updated.
- Prefer small, composable uniforms over magic constants scattered in code.
- Keep visual experiments readable and deterministic enough for debugging.
- If you add more shader stages, extend `Shader` cleanly rather than bypassing it.

## External Dependencies

- Do not edit vendored GLFW sources unless the user explicitly asks.
- Do not hand-edit generated GLAD files unless regeneration is impossible.
- If GLAD needs changes, regenerate it and document the inputs used.
- Keep `external/README.md` synchronized with actual dependency layout.

## Documentation Expectations

- Update `README.md` when build, run, debug, or dependency steps change.
- Keep commands copy-pasteable on Windows.
- Mention both Ninja and Visual Studio workflows when applicable.
- Document new tests, presets, or required tools as they are added.

## Git Workflow For Agents

- This repository is intended for local git use only unless the user adds a remote later.
- Agents may inspect history and workspace state with `git status`, `git diff`, and `git log`.
- Agents may stage files and create local commits when the user explicitly asks for a commit.
- Do not add a remote or push anywhere unless the user explicitly asks.
- Do not rewrite history unless the user explicitly asks.
- Never discard user changes just to make a task easier.
- Keep generated build output out of commits.

## Change Checklist

- Build the affected preset after code changes.
- Run relevant CTest coverage after adding or changing behavior.
- Verify shaders still copy beside the executable.
- Keep VS Code tasks and launch profiles valid if build layout changes.
- Keep agent guidance in this file current when workflow rules change.
