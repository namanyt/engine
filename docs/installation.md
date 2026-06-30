# Installation Guide

This guide covers different ways to install and integrate the Engine library into your project.

## System Requirements

### Minimum Requirements
- **OS**: Windows 10+, Linux (Ubuntu 20.04+), or macOS 10.15+
- **CMake**: 3.23 or higher
- **Compiler**: C++17 compatible (GCC 9+, Clang 10+, MSVC 2019+)
- **Graphics**: OpenGL 3.3 Core Profile capable GPU

### Recommended Requirements
- **OS**: Windows 11 or Ubuntu 22.04+
- **CMake**: 3.25 or higher
- **Compiler**: GCC 11+, Clang 13+, or MSVC 2022
- **Graphics**: Modern GPU with Vulkan support (for future compatibility)

## Installation Methods

### Method 1: Git Submodule (Recommended)

This method allows easy updates and keeps the engine version controlled.

```bash
# Add engine as a submodule
git submodule add https://github.com/youruser/engine-lib.git external/engine

# Initialize and update submodules
git submodule update --init --recursive
```

### Method 2: Direct Download

Download the latest release from GitHub:

```bash
# Clone the repository
git clone https://github.com/youruser/engine-lib.git external/engine

# Or download a specific version
wget https://github.com/youruser/engine-lib/archive/v1.0.0.tar.gz
tar -xzf v1.0.0.tar.gz
mv engine-lib-1.0.0 external/engine
```

### Method 3: Package Manager (Coming Soon)

Future support for package managers like:
- **Conan**: `conan install engine/1.0.0@youruser/stable`
- **vcpkg**: `vcpkg install engine`
- **Homebrew** (macOS): `brew install engine`

## Building the Engine

### Prerequisites Installation

#### Windows (MinGW)
```bash
# Install MinGW via MSYS2 or Chocolatey
choco install mingw cmake ninja
```

#### Ubuntu/Debian
```bash
sudo apt-get install build-essential cmake ninja-build \
    libglfw3-dev libgl1-mesa-dev libglu1-mesa-dev \
    libopenal-dev libsndfile-dev
```

#### macOS
```bash
brew install cmake ninja glfw3 glew openal-soft
```

### Build Configuration

Navigate to the engine directory and configure:

```bash
cd external/engine

# Configure with Ninja (recommended)
cmake -S . -B build -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Release \
    -DENGINE_BUILD_EXAMPLES=ON \
    -DENGINE_BUILD_TESTS=ON

# Build the engine
cmake --build build --parallel $(nproc)
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Debug | Build configuration (Debug/Release) |
| `ENGINE_BUILD_EXAMPLES` | ON | Build example applications |
| `ENGINE_BUILD_TESTS` | ON | Build unit tests |
| `ENGINE_BUILD_DOCS` | OFF | Build documentation |
| `ENGINE_USE_IMGUI` | ON | Enable ImGui integration |

**Note:** 3D model parsing (.obj, .gltf) is not yet implemented. Models are loaded as raw binary data only.

## Integration with Your Project

### CMake Integration

Add the engine to your project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.23)
project(MyGame LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add engine as subdirectory
add_subdirectory(external/engine EXCLUDE_FROM_ALL)

# Create your game executable
add_executable(mygame src/main.cpp)

# Link against engine
target_link_libraries(mygame PRIVATE engine)

# Set include directories
target_include_directories(mygame PRIVATE external/engine/include)

# Copy assets to build directory
add_custom_command(TARGET mygame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:mygame>/assets
)
```

### Manual Integration

If you prefer not to use CMake, link against the engine library directly:

```cpp
// Include engine headers
#include <engine/engine.hpp>

// Link libraries (varies by platform)
// Windows: -lengine -lglfw3 -lopengl32 -lopenal32
// Linux: -lengine -lglfw -lGL -lopenal
// macOS: -lengine -lglfw -framework OpenGL -framework OpenAL
```

## Verifying Installation

### Build the Examples

```bash
cd external/engine
cmake --build build --target atmospheric_demo
```

Run the example to verify everything works:

```bash
# Windows
.\build\bin\atmospheric_demo.exe

# Linux/macOS
./build/bin/atmospheric_demo
```

### Run the Tests

```bash
cd external/engine
ctest --build . -R smoke
```

All tests should pass for a successful installation.

## Troubleshooting

### Common Issues

#### CMake Configuration Errors
- Ensure CMake 3.23+ is installed: `cmake --version`
- Check that all dependencies are available
- Try cleaning the build directory: `rm -rf build/`

#### OpenGL Not Found
- Verify your GPU supports OpenGL 3.3
- Install graphics drivers
- Check if GLFW is properly installed

#### Missing Dependencies
- Run the platform-specific prerequisite installation commands
- Ensure development packages are installed (e.g., `-dev` on Ubuntu)

#### Build Fails with C++17 Errors
- Update your compiler to support C++17
- Verify `CMAKE_CXX_STANDARD 17` is set in CMakeLists.txt

## Next Steps

After successful installation:
1. [Quick Start Guide](quickstart.md) - Create your first game
2. [Architecture Overview](architecture.md) - Understand the engine design
3. [Runtime System](runtime/overview.md) - Learn about game states
4. [Renderer Documentation](renderer/overview.md) - Graphics programming
