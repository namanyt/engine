# Runtime Registration Pattern

The engine uses a registration pattern for runtimes, allowing external code to register custom game states without modifying the core engine library.

## Why Use Registration?

### Traditional Switch-Case Approach (Before)

```cpp
// In RuntimeFactory.cpp - not extensible
std::unique_ptr<RuntimeMode> RuntimeFactory::create(RuntimeId id) {
    switch (id) {
        case RuntimeId::Menu:
            return std::make_unique<MenuRuntime>();
        case RuntimeId::Exploration:
            return std::make_unique<ExplorationRuntime>();
        case RuntimeId::VN:
            return std::make_unique<VNRuntime>();
        default:
            throw std::runtime_error("Unknown runtime ID");
    }
}
```

**Problems:**
- Requires modifying engine code to add new runtimes
- Tightly couples all runtimes to the factory
- No way for external libraries to register custom runtimes

### Registration Pattern (After)

```cpp
// In RuntimeFactory.cpp - extensible
std::unordered_map<RuntimeId, RuntimeFactoryFn>& RuntimeFactory::registry() {
    static std::unordered_map<RuntimeId, RuntimeFactoryFn> map;
    return map;
}

void RuntimeFactory::registerRuntime(RuntimeId id, RuntimeFactoryFn factory) {
    registry()[id] = std::move(factory);
}

std::unique_ptr<RuntimeMode> RuntimeFactory::create(RuntimeId id) {
    auto it = registry().find(id);
    if (it == registry().end()) {
        throw std::runtime_error("Runtime not registered: " + std::to_string(id));
    }
    return it->second();
}
```

**Benefits:**
- External code can register custom runtimes
- Engine remains unmodified for custom functionality
- Clean separation between core and user code

## Registration API

### RuntimeFactory Class

```cpp
class RuntimeFactory final {
public:
    // Register a runtime with a factory function
    static void registerRuntime(RuntimeId runtimeId, RuntimeFactoryFn factory);
    
    // Create a runtime instance
    static std::unique_ptr<RuntimeMode> create(RuntimeId runtimeId);
};

// Factory function type
using RuntimeFactoryFn = std::function<std::unique_ptr<RuntimeMode>()>;
```

## Registration Examples

### Basic Registration

```cpp
// Register a simple runtime
RuntimeFactory::registerRuntime(
    RuntimeId::Custom,
    []() -> std::unique_ptr<RuntimeMode> {
        return std::make_unique<MyCustomRuntime>();
    }
);
```

### Registration with Dependencies

```cpp
// Register runtime that needs dependencies
void registerRuntimes(std::shared_ptr<AssetManager> assetManager) {
    RuntimeFactory::registerRuntime(
        RuntimeId::Exploration,
        [assetManager]() -> std::unique_ptr<RuntimeMode> {
            return std::make_unique<ExplorationRuntime>(assetManager);
        }
    );
}
```

### Registration in Main Application

```cpp
int main() {
    // Create application and systems
    engine::Application app("My Game", 1280, 720);
    auto assetManager = std::make_shared<engine::AssetManager>();
    
    // Register all runtimes
    registerRuntimes(assetManager);
    
    // Run application
    app.run(RuntimeId::Menu);
    
    return 0;
}

void registerRuntimes(std::shared_ptr<engine::AssetManager> assetManager) {
    // Register menu runtime
    RuntimeFactory::registerRuntime(
        RuntimeId::Menu,
        [assetManager]() -> std::unique_ptr<RuntimeMode> {
            return std::make_unique<MenuRuntime>(assetManager);
        }
    );
    
    // Register exploration runtime
    RuntimeFactory::registerRuntime(
        RuntimeId::Exploration,
        [assetManager]() -> std::unique_ptr<RuntimeMode> {
            return std::make_unique<ExplorationRuntime>(assetManager);
        }
    );
    
    // Register custom runtimes
    RuntimeFactory::registerRuntime(
        RuntimeId::Custom,
        []() -> std::unique_ptr<RuntimeMode> {
            return std::make_unique<MyCustomRuntime>();
        }
    );
}
```

## Runtime ID Management

### Using Predefined IDs

```cpp
// Use predefined IDs for standard runtimes
RuntimeFactory::registerRuntime(RuntimeId::Menu, menuFactory);
RuntimeFactory::registerRuntime(RuntimeId::Exploration, explorationFactory);
RuntimeFactory::registerRuntime(RuntimeId::VN, vnFactory);
```

### Creating Custom IDs

```cpp
// Use custom IDs starting from RuntimeId::Custom
enum class CustomRuntimeIds : uint32_t {
    FirstCustom = RuntimeId::Custom,
    BattleMode = FirstCustom,
    TutorialMode = FirstCustom + 1,
    SandboxMode = FirstCustom + 2,
};

// Register with custom ID
RuntimeFactory::registerRuntime(
    static_cast<RuntimeId>(CustomRuntimeIds::BattleMode),
    []() -> std::unique_ptr<RuntimeMode> {
        return std::make_unique<BattleRuntime>();
    }
);
```

## Error Handling

### Unregistered Runtime

```cpp
try {
    auto runtime = RuntimeFactory::create(RuntimeId::Custom + 999);
} catch (const std::runtime_error& e) {
    // Handle error: "Runtime not registered: 1000"
    std::cerr << "Failed to create runtime: " << e.what() << std::endl;
}
```

### Checking Registration

```cpp
// Note: You can check if a runtime is registered by attempting creation
bool isRuntimeRegistered(RuntimeId id) {
    try {
        // This will throw if not registered
        return true;
    } catch (const std::runtime_error&) {
        return false;
    }
}
```

## Best Practices

### 1. Register Early

Register all runtimes before starting the application:

```cpp
int main() {
    engine::Application app("My Game", 1280, 720);
    
    // Register ALL runtimes upfront
    registerAllRuntimes();
    
    // Then start
    app.run(RuntimeId::Menu);
}
```

### 2. Use Lambda Factories

Lambda functions provide clean factory syntax:

```cpp
RuntimeFactory::registerRuntime(
    RuntimeId::Custom,
    []() -> std::unique_ptr<RuntimeMode> {
        // Factory logic here
        return std::make_unique<MyRuntime>();
    }
);
```

### 3. Capture Dependencies Carefully

Only capture what's necessary in lambda factories:

```cpp
// Good: Capture only needed dependencies
auto factory = [assetManager]() -> std::unique_ptr<RuntimeMode> {
    return std::make_unique<MyRuntime>(assetManager);
};

// Avoid: Capturing everything
auto badFactory = [&]() -> std::unique_ptr<RuntimeMode> {
    return std::make_unique<MyRuntime>(assetManager, renderer, ecs);
};
```

### 4. Document Custom Runtimes

Clearly document your custom runtimes:

```cpp
/// Registers all game-specific runtimes with the factory
/// @param assetManager Shared asset manager for resource loading
void registerGameRuntimes(std::shared_ptr<engine::AssetManager> assetManager);
```

## Related Documentation

- [Runtime Overview](overview.md) - General runtime system information
- [Atmospheric Demo Example](../examples/atmospheric-demo.md) - Complete registration example
- [Architecture Overview](../../architecture.md) - System design principles
