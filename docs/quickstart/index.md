# Quick Start Guide

This guide will help you get started with using the Engine library in your project.

## Prerequisites

- CMake 3.23 or higher
- A C++17 compatible compiler (GCC, Clang, or MSVC)
- OpenGL 3.3 Core Profile support

## Installation

### Option 1: Using as a Submodule

```bash
git submodule add https://github.com/youruser/engine-lib.git external/engine
git submodule update --init --recursive
```

### Option 2: Downloading the Library

Download the latest release and extract to your project's `external/` directory.

## Building the Engine Library

Navigate to the engine directory and configure with CMake:

```bash
cd external/engine
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Using Engine in Your Project

### CMake Integration

Add this to your `CMakeLists.txt`:

```cmake
# Add the engine as a subdirectory
add_subdirectory(external/engine EXCLUDE_FROM_ALL)

# Create your executable
add_executable(mygame src/main.cpp)

# Link against the engine library
target_link_libraries(mygame PRIVATE engine)

# Set include directories
target_include_directories(mygame PRIVATE external/engine/include)
```

### Basic Application Structure

Here's a minimal application using the Engine:

```cpp
#include <engine/engine.hpp>
#include <GLFW/glfw3.h>

class MyRuntime : public engine::RuntimeMode {
public:
    void initialize(engine::Application& app, engine::Renderer& renderer) override {
        // Initialize your runtime
    }

    void update(float deltaTime, engine::Application& app, engine::Renderer& renderer) override {
        // Update game logic
    }

    void render(engine::Renderer& renderer) override {
        // Render your scene
    }

    void shutdown() override {
        // Cleanup resources
    }
};

int main() {
    // Create application
    engine::Application app("My Game", 1280, 720);

    // Register custom runtime
    engine::RuntimeFactory::registerRuntime(
        engine::RuntimeId::Custom,
        []() -> std::unique_ptr<engine::RuntimeMode> {
            return std::make_unique<MyRuntime>();
        }
    );

    // Start application
    app.run(engine::RuntimeId::Custom);

    return 0;
}
```

## Current Limitations

- **3D Model Parsing**: .obj and .gltf files are loaded as raw binary data only. Full parsing (vertices, faces, normals) is not yet implemented.
- **Debug Build**: Requires ImGui dependency to be installed.

## Next Steps

- Read the [Architecture Overview](../architecture.md) to understand the engine's design
- Explore the [Runtime System](../runtime/overview.md) for game state management
- Learn about the [Renderer](../renderer/overview.md) for graphics
- Check out the [ECS System](../ecs/overview.md) for entity management
- Review the [Asset Manager](../assets/overview.md) for resource handling
- Study the [Atmospheric Demo](../examples/atmospheric-demo.md) example
