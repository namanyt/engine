# EngineStarter

Minimal OpenGL 3.3 Core starter project for Windows using CMake, GLFW, and GLAD.

The repository now includes vendored GLFW source in `external/glfw` and generated GLAD files in `external/glad`.

## Included systems

- `Application`: window lifecycle, context setup, main loop, input handling
- `Renderer`: GPU buffer setup, shader binding, draw calls
- `Shader`: file loading, compilation, program linking, uniform updates
- `shaders/`: external GLSL files copied next to the executable after build

## Dependency setup

Dependencies are already present in this workspace. If you need to recreate them elsewhere, follow `external/README.md`. The build expects:

- `external/glfw/`
- `external/glad/include/`
- `external/glad/src/glad.c`

## Configure and build with Visual Studio

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

Run the executable from `build/bin/Debug/EngineStarter.exe`.

The Visual Studio 2022 Debug workflow was verified successfully in this workspace.

## Configure and build with Ninja

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

Run the executable from `build/bin/EngineStarter.exe`.

The Ninja workflow was verified successfully in this workspace.

## Configure and build with presets

```powershell
cmake --preset debug-ninja
cmake --build --preset build-debug-ninja
ctest --preset test-debug-ninja --output-on-failure
```

Single smoke test example:

```powershell
ctest --preset test-debug-ninja -R shader_assets_exist --output-on-failure
```

## VS Code setup

- Open the repository root in VS Code.
- Install the recommended extensions when prompted.
- CMake Tools is configured to use `CMakePresets.json` automatically.
- The default development workflow uses the `debug-ninja` preset.
- Shared tasks are available for configure, build, run, full test, and single-test execution.

## Runtime behavior

- Opens an `800x600` window
- Creates an OpenGL 3.3 Core context
- Loads GLAD after the context is current
- Renders a triangle through a VAO and VBO
- Uses a time-driven fragment shader for an animated greeting-like color effect
- Closes when `Escape` is pressed

## Tests

- `shader_assets_exist`: verifies required GLSL files are present
- `dependency_layout_exists`: verifies GLFW and GLAD files exist in `external/`
- `engine_source_layout_exists`: verifies expected starter source files exist

## Common issues

- Blank screen: confirm shaders were copied to `build/bin/.../shaders` and check startup logs for shader errors.
- GLAD failure: verify generated GLAD files target OpenGL 3.3 Core and that `glad.c` is present.
- Old OpenGL version: ensure the GPU driver supports OpenGL 3.3 and the window hints are not overridden.

## OpenGL version check

The application prints vendor, renderer, and OpenGL version strings on startup. Use that output to verify the expected driver and profile are active.
