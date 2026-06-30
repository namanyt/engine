# Engine Library

A C++ game engine library for atmospheric exploration projects.

## Overview

Engine is a custom C++ runtime and rendering framework designed for creating atmospheric, exploration-focused games. The engine provides a modular architecture with clear separation between core systems and application code.

## Features

- **OpenGL 3.3 Core Renderer** - Modern graphics pipeline with VAOs and shaders
- **Runtime Architecture** - Scene and mode management via registration API
- **Atmospheric Rendering** - Volumetric fog, HDR post-processing, bloom effects
- **Entity-Component System** - ECS-driven world systems for game objects
- **Asset Management** - Runtime asset loading with lazy shader compilation
- **Scene Transitions** - Seamless runtime scene transitions with loading previews
- **Debug Tools** - ImGui-based debug UI and profiling tools (debug builds)
- **Procedural Generation** - Environment generation and primitive geometry

## Architecture

```
engine/
├── include/engine/          # Public API headers
│   ├── engine.hpp           # Main entry point
│   ├── runtime.hpp          # Runtime factory and modes
│   ├── renderer.hpp         # OpenGL rendering pipeline
│   ├── ecs.hpp               # Entity-component-system
│   └── assets.hpp            # Asset management
├── src/                     # Engine library source
│   ├── core/                 # Application, logging
│   ├── renderer/             # Rendering pipeline
│   ├── ecs/                  # ECS implementation
│   ├── assets/               # Asset management
│   ├── geometry/             # Geometry operations
│   ├── graphics/             # GPU wrappers (Mesh, buffers)
│   ├── math/                 # Vector/matrix types
│   └── primitives/           # Shape generators
├── examples/atmospheric_demo/  # Comprehensive demo application
├── tests/                   # Unit tests
└── external/                # Third-party dependencies
```

## Build Requirements

- C++17 compliant compiler (GCC 13+, MSVC 2022+)
- CMake 3.23+
- Ninja build system (recommended)
- OpenGL development libraries
- GLFW 3 (vendored in external/glfw)
- GLAD (vendored in external/glad)

## Building

### Configure

```powershell
# Using Ninja (recommended)
cmake --preset release-ninja

# Using Visual Studio 2022 x64
cmake --preset vs2022-x64
```

### Build Targets

```powershell
# Build engine library only
cmake --build --preset build-release-ninja --target engine

# Build atmospheric demo example
cmake --build --preset build-release-ninja --target atmospheric_demo

# Build all targets
cmake --build --preset build-release-ninja
```

### Run Tests

```powershell
ctest --preset test-release-ninja --output-on-failure
```

## Usage

### Basic Example

```cpp
#include <engine/engine.hpp>

int main() {
    // Register runtimes before creating engine
    engine::RuntimeFactory::registerRuntime(engine::RuntimeId::Menu, []() {
        return std::make_unique<MenuRuntime>();
    });
    
    // Create and run engine runtime
    EngineRuntime runtime;
    return runtime.run();
}
```

### Using the Engine Library

Link against the `engine` library in your CMakeLists.txt:

```cmake
target_link_libraries(your_app PRIVATE engine)
```

## Examples

The `examples/atmospheric_demo/` directory contains a comprehensive demo application showcasing:

- Menu system with settings overlay
- Atmospheric exploration runtime
- Visual novel prototype system
- Scene transitions and loading screens
- Player movement and camera controls

## License

See [LICENSE](LICENSE) for details.
