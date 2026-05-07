# EngineStarter

Minimal OpenGL 3.3 Core starter project for Windows using CMake, GLFW, and GLAD.

The repository now includes vendored GLFW source in `external/glfw` and generated GLAD files in `external/glad`.
Debug builds can also use Dear ImGui from `external/imgui` for the in-engine debug menu.

## Included systems

- `Application`: window lifecycle, context setup, swap/poll/input handling
- `core/Renderer`: frame setup and mesh draw submission from caller-provided frame uniforms
- `core/Shader`: file loading, compilation, program linking, uniform updates
- `geometry/`: CPU-side geometry types, 2D generators, and geometry operations like extrusion
- `graphics/`: thin OpenGL wrappers for VAO/VBO/EBO plus `Mesh`
- `primitives/`: reusable basic shapes including `Triangle`, `Quad`, `Plane`, `Cube`, `Pyramid`, `Sphere`, `Cylinder`, `Cone`, `Capsule`, and `Torus`
- `math/`: foundational vector, color, matrix, and transform helpers
- `world/`: camera, free-fly controller, scene container, and atmospheric test-world assembly
- `shaders/`: external GLSL files copied next to the executable after build

## Dependency setup

Dependencies are already present in this workspace. If you need to recreate them elsewhere, follow `external/README.md`. The build expects:

- `external/glfw/`
- `external/glad/include/`
- `external/glad/src/glad.c`
- `external/imgui/` for debug UI builds

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

- Opens a `1600x900` window
- Creates an OpenGL 3.3 Core context
- Disables VSync by default so the frame rate is uncapped
- Loads GLAD after the context is current
- Captures the mouse for free-look camera control on startup
- Builds an atmospheric world-space scene with a large ground plane, distant structures, and fog falloff
- Uses caller-provided camera view/projection state so world systems stay outside the renderer
- Applies distance fog in the fragment shader to soften far geometry and hide scene bounds
- In debug UI builds, `Escape` releases the mouse and a second `Escape` resumes camera capture when the menu is open
- In release-style builds without the debug UI, `Escape` closes the application

## Controls

- `W`, `A`, `S`, `D`: move on the ground plane relative to the camera heading
- `Space`: move upward
- `Left Ctrl`: move downward
- `Left Shift`: sprint
- Mouse: look around
- `Escape`: release or recapture the mouse in debug UI builds, or close the app otherwise
- `F1`: toggle the debug menu in debug builds

## Tests

- `shader_assets_exist`: verifies required GLSL files are present
- `dependency_layout_exists`: verifies GLFW and GLAD files exist in `external/`
- `engine_source_layout_exists`: verifies expected starter source files exist, including the world subsystem

## Primitive coverage

- Built-in primitive classes now cover common engine starter shapes: triangle, quad, plane, cube, pyramid, sphere, cylinder, cone, capsule, and torus.
- Mesh vertices now carry position, normal, UV, and color data so later material and texture work can build on the same format.
- Core 2D procedural generators are `makeTriangle()`, `makeQuad()`, and `makeCircle(int)`.
- Core geometry operation is `extrude(const Geometry&, float)`, which is now used to derive 3D solids like cubes and cylinders from 2D source geometry.

## Common issues

- Blank screen: confirm shaders were copied to `build/bin/.../shaders` and check startup logs for shader errors.
- GLAD failure: verify generated GLAD files target OpenGL 3.3 Core and that `glad.c` is present.
- Old OpenGL version: ensure the GPU driver supports OpenGL 3.3 and the window hints are not overridden.

## OpenGL version check

The application prints vendor, renderer, and OpenGL version strings on startup. Use that output to verify the expected driver and profile are active.
