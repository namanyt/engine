# Architecture Overview

The Engine library is designed as a modular, reusable game engine that separates concerns into distinct subsystems. This architecture allows for clean integration into different game projects while maintaining a clear separation of responsibilities.

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                        Application                      │
│   - GLFW window management                              │
│   - Input handling                                      │
│   - Event polling                                       │
│   - Frame timing                                        │
└──────────────────────────────┬──────────────────────────┘
                               │
                               ▼
┌──────────────────────────────▼──────────────────────────┐
│                      Runtime Factory                    │
│   - Runtime registration                                │
│   - State machine management                            │
│   - Runtime creation/destruction                        │
└──────────────────────────────┬──────────────────────────┘
                               │
        ┌──────────────────────┼─────────────────────┐
        ▼                      ▼                     ▼
┌───────▼───────┐       ┌─────▼───────┐         ┌─────▼──────┐
│   Renderer    │       │    ECS      │         │  Assets    │
│               │       │             │         │            │
│ • Frame setup │       │ • Entities  │         │ • Manager  │
│ • Shaders     │       │ • Registry  │         │ • Discovery│
│ • Draw calls  │       │ • Systems   │         │ • Loading  │
│ • Post FX     │       │ • Components│         │ • Metadata │
└───────┬───────┘       └─────┬───────┘         └─────┬──────┘
        │                     │                       │
        ▼                     ▼                       ▼
┌───────▼───────┐       ┌─────▼─────┐           ┌─────▼─────┐
│    OpenGL     │       │  Memory   │           │   Files   │
│ • VAO/VBO     │       │ • Pools   │           │ • Disk I/O│
│ • Textures    │       │ • Systems │           │ • Formats │
│ • Framebuffers│       │           │           │           │
└───────────────┘       └───────────┘           └───────────┘
```

## Core Subsystems

### 1. Application Layer

The `Application` class manages the platform-specific aspects of game development:

- **Window Management**: Creates and manages GLFW windows
- **Input Handling**: Processes keyboard, mouse, and gamepad input
- **Event Polling**: Handles window events (resize, close, focus)
- **Frame Timing**: Provides delta time for smooth animation

```cpp
// Example application setup
engine::Application app("My Game", 1280, 720);
app.setVSync(true);
app.run(engine::RuntimeId::Custom);
```

### 2. Runtime System

The runtime system manages game states through a registration pattern:

- **RuntimeFactory**: Creates and switches between different game states
- **Registration Pattern**: Allows external code to register custom runtimes
- **State Management**: Handles transitions and lifecycle events

```cpp
// Register a custom runtime
engine::RuntimeFactory::registerRuntime(
    engine::RuntimeId::Custom,
    []() -> std::unique_ptr<engine::RuntimeMode> {
        return std::make_unique<MyGameRuntime>();
    }
);
```

### 3. Rendering System

The renderer handles all graphics operations:

- **Frame Management**: Begins and ends rendering frames
- **Shader Management**: Loads, compiles, and manages GLSL shaders
- **Draw Submission**: Handles mesh rendering with materials
- **Post-Processing**: Implements bloom, tonemapping, and other effects
- **Shadow Mapping**: Generates and applies shadow maps

```cpp
// Basic rendering
renderer.beginFrame(Color{0.1f, 0.2f, 0.3f});
renderer.draw(mesh, material, transform, frameUniforms);
renderer.endFrame(postProcessSettings, frameUniforms, timeSeconds);
```

### 4. Entity Component System (ECS)

The ECS provides a data-oriented approach to game entity management:

- **Entities**: Lightweight identifiers for game objects
- **Components**: Data containers for entity properties
- **Registry**: Manages entity lifecycle and component storage
- **Systems**: Process entities based on their components

```cpp
// Create an entity with components
auto entity = registry.createEntity();
registry.addComponent<PositionComponent>(entity, {x, y, z});
registry.addComponent<RotationComponent>(entity, {pitch, yaw, roll});
```

### 5. Asset Management

The asset system handles resource discovery and loading:

- **Discovery**: Automatically finds assets in directories
- **Loading**: Lazy loading with caching support
- **Metadata**: Generates and maintains asset information
- **Type Safety**: Compile-time type checking for asset types

```cpp
// Discover and load assets
assetManager.discover("assets");
auto texture = assetManager.load<TextureAsset>("textures/sky.png");
auto audio = assetManager.load<AudioAsset>("audio/music.ogg");
```

## Design Principles

### 1. Separation of Concerns

Each subsystem has a clear responsibility:

- **Application**: Platform and window management
- **Runtime**: Game state logic
- **Renderer**: Graphics operations
- **ECS**: Entity and component data
- **Assets**: Resource management

### 2. Dependency Injection

Subsystems receive their dependencies through constructors:

```cpp
// Renderer receives asset manager and shader path
auto renderer = std::make_unique<engine::Renderer>(
    assetManager,
    std::filesystem::path("assets/shaders")
);
```

### 3. Resource Ownership

Clear ownership rules prevent memory issues:

- `std::unique_ptr` for exclusive ownership
- `std::shared_ptr` for shared resources
- RAII patterns for automatic cleanup

### 4. Extensibility

The engine supports extension through:

- Runtime registration pattern
- Custom component types
- Pluggable asset loaders
- Shader-based effects

## File Organization

```
engine/
├── include/engine/          # Public API headers
│   ├── engine.hpp           # Main entry point
│   ├── runtime.hpp          # Runtime system
│   ├── renderer.hpp         # Rendering subsystem
│   ├── ecs.hpp              # Entity component system
│   └── assets.hpp           # Asset management
├── src/                     # Implementation files
│   ├── core/                # Core rendering components
│   ├── runtime/             # Runtime factory and states
│   ├── ecs/                 # ECS implementation
│   ├── assets/              # Asset management
│   ├── geometry/            # Geometry operations
│   ├── graphics/            # OpenGL wrappers
│   ├── math/                # Mathematical utilities
│   └── world/               # World simulation
├── examples/                # Example applications
└── tests/                   # Unit tests
```

## Thread Safety Considerations

- **Main Thread**: All OpenGL operations must occur on the main thread
- **Asset Loading**: Can be performed on worker threads with proper synchronization
- **ECS Systems**: Generally safe for parallel execution when properly designed
- **Runtime Switching**: Occurs on main thread to ensure clean state transitions

## Performance Characteristics

The engine is optimized for:

- **Low Latency**: Minimal overhead in frame rendering path
- **Memory Efficiency**: Object pooling and cache-friendly data layouts
- **Scalability**: Support for large entity counts through ECS architecture
- **Resource Management**: Lazy loading and automatic cleanup
