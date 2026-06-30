# Runtime System Overview

The runtime system manages game states and provides a framework for handling different modes of operation within your game. It uses a registration pattern that allows external code to define and register custom runtimes.

## What is a Runtime?

A runtime represents a distinct mode of operation in your game, such as:

- **Menu**: Main menu, options screens, level selection
- **Exploration**: Open world exploration mode
- **Visual Novel (VN)**: Story-driven narrative sequences
- **Loading**: Resource loading and transitions
- **Custom**: User-defined game states

## System Architecture

```
┌─────────────────────────┐
│     Application         │
│                         │
│  • Window management    │
│  • Input handling       │
│  • Frame timing         │
└───────────┬─────────────┘
            │
            ▼
┌─────────────────────────┐
│    Runtime Factory      │
│                         │
│  • Registration map     │
│  • Runtime creation     │
│  • State transitions    │
└────────────────┬────────┘
                 │
    ┌────────────┼───────────┐
    ▼            ▼           ▼
┌───────┐  ┌───────────┐ ┌────────┐
│ Menu  │  │Exploration│ │VN      │
│Runtime│  │ Runtime   │ │Runtime │
└───────┘  └───────────┘ └────────┘
```

## Runtime Lifecycle

Each runtime follows a specific lifecycle:

```
                    ┌─────────────┐
                    │   Created   │
                    └──────┬──────┘
                           │ initialize()
                           ▼
                    ┌─────────────┐
                    │ Initialized │◄───────────────────┐
                    └──────┬──────┘                    │
                           │ update()                  │
                           ▼                           │
                    ┌─────────────┐                    │
                    │    Active   │────────────────────┘
                    └──────┬──────┘
                           │ render()
                           ▼
                    ┌─────────────┐
                    │   Rendered  │
                    └──────┬──────┘
                           │ shutdown()
                           ▼
                    ┌─────────────┐
                    │   Destroyed │
                    └─────────────┘
```

## Runtime Interface

The `RuntimeMode` base class defines the runtime interface:

```cpp
class RuntimeMode {
public:
    virtual ~RuntimeMode() = default;

    // Called when the runtime is first created
    virtual void initialize(Application& app, Renderer& renderer) = 0;

    // Called every frame with delta time
    virtual void update(float deltaTime, Application& app, Renderer& renderer) = 0;

    // Called to render the current frame
    virtual void render(Renderer& renderer) = 0;

    // Called when the runtime is being destroyed
    virtual void shutdown() = 0;
};
```

## Runtime IDs

The engine provides predefined runtime identifiers:

```cpp
enum class RuntimeId : uint32_t {
    Menu = 1,           // Main menu runtime
    Exploration = 2,    // Open world exploration
    VN = 3,             // Visual novel mode
    Loading = 4,        // Loading screen
    TestWorld = 5,      // Test/debug world
    Custom = 100,       // Starting point for custom runtimes
};
```

## Creating a Custom Runtime

### Step 1: Define the Runtime Class

```cpp
class MyGameRuntime : public engine::RuntimeMode {
public:
    void initialize(engine::Application& app, engine::Renderer& renderer) override {
        // Initialize resources
        m_renderer = &renderer;

        // Load assets
        auto assetManager = app.assetManager();
        m_texture = assetManager->load<engine::TextureAsset>("textures/background.png");

        // Setup ECS entities
        setupEntities(app);
    }

    void update(float deltaTime, engine::Application& app, engine::Renderer& renderer) override {
        // Update game logic
        updatePlayer(deltaTime, app);
        updatePhysics(deltaTime);
        updateAI(deltaTime);
    }

    void render(engine::Renderer& renderer) override {
        // Render scene
        renderer.beginFrame(Color{0.1f, 0.2f, 0.3f});

        renderWorld(renderer);
        renderUI(renderer);

        renderer.endFrame(postProcessSettings, frameUniforms, timeSeconds);
    }

    void shutdown() override {
        // Clean up resources
        m_texture.reset();
        cleanupEntities();
    }

private:
    engine::Renderer* m_renderer = nullptr;
    std::shared_ptr<engine::TextureAsset> m_texture;

    void setupEntities(engine::Application& app);
    void updatePlayer(float deltaTime, engine::Application& app);
    void updatePhysics(float deltaTime);
    void updateAI(float deltaTime);
    void renderWorld(engine::Renderer& renderer);
    void renderUI(engine::Renderer& renderer);
    void cleanupEntities();
};
```

### Step 2: Register the Runtime

```cpp
// In your main.cpp or initialization code
engine::RuntimeFactory::registerRuntime(
    engine::RuntimeId::Custom,
    []() -> std::unique_ptr<engine::RuntimeMode> {
        return std::make_unique<MyGameRuntime>();
    }
);
```

### Step 3: Start the Runtime

```cpp
int main() {
    engine::Application app("My Game", 1280, 720);

    // Register custom runtimes
    registerRuntimes();

    // Start with menu runtime, then switch to custom
    app.run(engine::RuntimeId::Menu);

    return 0;
}
```

## State Transitions

The runtime system supports smooth transitions between states:

```cpp
// Transition to another runtime
app.transitionToRuntime(engine::RuntimeId::Exploration);

// With transition animation
app.transitionWithAnimation(
    engine::RuntimeId::VN,
    engine::TransitionType::Fade,
    0.5f  // Duration in seconds
);
```

## Best Practices

### Resource Management

- Load heavy resources during `initialize()`
- Unload resources in `shutdown()`
- Use the asset manager for automatic resource tracking

### Performance

- Minimize work in `update()` - only process what's necessary
- Batch rendering operations in `render()`
- Use delta time for frame-rate independent updates

### State Management

- Keep runtime state minimal and focused
- Store persistent data in shared systems (ECS, asset manager)
- Avoid direct coupling between runtimes

## Example Runtime Implementations

The engine includes several example runtimes:

1. **MenuRuntime**: Main menu with options and level selection
2. **ExplorationRuntime**: Open world exploration with camera controls
3. **VNRuntime**: Visual novel mode for story sequences
4. **LoadingRuntime**: Resource loading with progress indicators

See the [Atmospheric Demo](../examples/atmospheric-demo.md) for complete implementations.

## Related Documentation

- [Registration Pattern](registration.md) - How to register custom runtimes
- [Renderer Integration](../renderer/overview.md) - Using the renderer in runtimes
- [ECS Integration](../ecs/overview.md) - Managing entities in runtimes
