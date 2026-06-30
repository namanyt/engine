# Atmospheric Demo Example

This document provides a comprehensive walkthrough of the Atmospheric Demo example application, demonstrating how to use the Engine library to create a complete game with multiple runtimes, ECS integration, and advanced rendering features.

## Project Overview

The Atmospheric Demo is a comprehensive example that showcases:
- Multiple runtime states (Menu, Exploration, Visual Novel, Loading)
- Entity Component System integration
- Advanced rendering pipeline with shadows and post-processing
- Asset management and discovery
- Camera controls and world simulation
- UI overlay systems

## Directory Structure

```
examples/atmospheric_demo/
├── src/
│   ├── main.cpp                     # Application entry point
│   ├── EngineRuntime.cpp/h          # Base runtime class
│   ├── MenuRuntime.cpp/h            # Main menu implementation
│   ├── ExplorationRuntime.cpp/h     # Open world exploration
│   ├── VNRuntime.cpp/h              # Visual novel mode
│   ├── LoadingRuntime.cpp/h         # Loading screen
│   │
│   ├── scenes/                      # Scene implementations
│   │   ├── AtmosphericSceneRuntime.h
│   │   ├── DaylightSandboxScene.cpp/h
│   │   └── LoadingScene.cpp/h
│   │
│   ├── metassets/                   # Metasset definitions
│   │   ├── DaylightSandboxScene.metasset.cpp/h
│   │   └── LoadingScene.metasset.cpp/h
│   │
│   ├── ui/                          # UI components
│   │   ├── SettingsOverlay.cpp/h
│   │   ├── SettingsPage.cpp/h
│   │   └── StartupFlowOverlay.cpp/h
│   │
│   └── scripts/                     # Script files
│       └── VnScript.cpp/h
│
├── assets/                          # Game assets
│   ├── textures/                    # Texture files
│   ├── audio/                       # Audio files
│   ├── models/                      # 3D model files
│   └── shaders/                     # Custom shaders
│
└── CMakeLists.txt                   # Build configuration
```

## Application Entry Point

### Main.cpp Structure

The main entry point demonstrates proper application initialization:

```cpp
#include <engine/engine.hpp>
#include "MenuRuntime.h"
#include "ExplorationRuntime.h"
#include "VNRuntime.h"
#include "LoadingRuntime.h"

// Register all available runtimes
void registerRuntimes() {
    // Register menu runtime
    engine::RuntimeFactory::registerRuntime(
        engine::RuntimeId::Menu,
        []() -> std::unique_ptr<engine::RuntimeMode> {
            return std::make_unique<MenuRuntime>();
        }
    );
    
    // Register exploration runtime
    engine::RuntimeFactory::registerRuntime(
        engine::RuntimeId::Exploration,
        []() -> std::unique_ptr<engine::RuntimeMode> {
            return std::make_unique<ExplorationRuntime>();
        }
    );
    
    // Register visual novel runtime
    engine::RuntimeFactory::registerRuntime(
        engine::RuntimeId::VN,
        []() -> std::unique_ptr<engine::RuntimeMode> {
            return std::make_unique<VNRuntime>();
        }
    );
    
    // Register loading runtime
    engine::RuntimeFactory::registerRuntime(
        engine::RuntimeId::Loading,
        []() -> std::unique_ptr<engine::RuntimeMode> {
            return std::make_unique<LoadingRuntime>();
        }
    );
}

int main() {
    try {
        // Create application
        engine::Application app("Atmospheric Demo", 1280, 720);
        
        // Configure application settings
        app.setVSync(true);
        app.setFullscreen(false);
        
        // Register runtimes
        registerRuntimes();
        
        // Start application with menu runtime
        app.run(engine::RuntimeId::Menu);
        
    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

## Runtime Implementations

### Menu Runtime

The menu runtime handles main menu functionality:

```cpp
class MenuRuntime : public engine::RuntimeMode {
public:
    void initialize(engine::Application& app, engine::Renderer& renderer) override {
        // Initialize menu assets
        auto assetManager = app.assetManager();
        
        // Load UI textures
        uiBackground = assetManager->load<engine::TextureAsset>("textures/ui/menu_bg.png");
        buttonTexture = assetManager->load<engine::TextureAsset>("textures/ui/button.png");
        
        // Load menu music
        menuMusic = assetManager->load<engine::AudioAsset>("audio/menu_music.ogg");
        
        // Setup ECS entities for UI elements
        setupMenuEntities(app);
    }
    
    void update(float deltaTime, engine::Application& app, engine::Renderer& renderer) override {
        // Handle menu navigation
        handleInput(app);
        
        // Update button animations
        updateButtons(deltaTime);
    }
    
    void render(engine::Renderer& renderer) override {
        // Render menu background
        renderer.beginFrame(Color{0.1f, 0.15f, 0.2f});
        
        // Render UI elements
        renderMenuUI(renderer);
        
        renderer.endFrame(postProcessSettings, frameUniforms, timeSeconds);
    }
    
    void shutdown() override {
        // Cleanup menu resources
        uiBackground.reset();
        buttonTexture.reset();
        menuMusic.reset();
    }

private:
    std::shared_ptr<engine::TextureAsset> uiBackground;
    std::shared_ptr<engine::TextureAsset> buttonTexture;
    std::shared_ptr<engine::AudioAsset> menuMusic;
    
    void setupMenuEntities(engine::Application& app);
    void handleInput(engine::Application& app);
    void updateButtons(float deltaTime);
    void renderMenuUI(engine::Renderer& renderer);
};
```

### Exploration Runtime

The exploration runtime handles open world gameplay:

```cpp
class ExplorationRuntime : public engine::RuntimeMode {
public:
    void initialize(engine::Application& app, engine::Renderer& renderer) override {
        // Initialize world assets
        auto assetManager = app.assetManager();
        
        // Load world textures
        skyTexture = assetManager->load<engine::TextureAsset>("textures/sky.png");
        groundTexture = assetManager->load<engine::TextureAsset>("textures/ground.png");
        
        // Note: Model loading is currently binary-only (no parsing)
        // characterModel = assetManager->load<engine::ModelAsset>("models/character.obj");
        
        // Setup world entities
        setupWorldEntities(app);
        
        // Initialize camera
        camera = std::make_unique<engine::Camera>(app);
    }
    
    void update(float deltaTime, engine::Application& app, engine::Renderer& renderer) override {
        // Update camera from input
        camera->updateFromInput(deltaTime, app);
        
        // Update player position
        updatePlayerMovement(deltaTime, app);
        
        // Update world systems
        updateWorldSystems(deltaTime, app);
    }
    
    void render(engine::Renderer& renderer) override {
        // Setup frame uniforms with camera data
        updateFrameUniforms(renderer);
        
        // Render world
        renderer.beginFrame(Color{0.1f, 0.2f, 0.3f});
        
        renderWorld(renderer);
        renderPlayer(renderer);
        
        renderer.endFrame(postProcessSettings, frameUniforms, timeSeconds);
    }
    
    void shutdown() override {
        // Cleanup world resources
        skyTexture.reset();
        groundTexture.reset();
        characterModel.reset();
    }

private:
    std::shared_ptr<engine::TextureAsset> skyTexture;
    std::shared_ptr<engine::TextureAsset> groundTexture;
    std::shared_ptr<engine::ModelAsset> characterModel;
    std::unique_ptr<engine::Camera> camera;
    
    void setupWorldEntities(engine::Application& app);
    void updatePlayerMovement(float deltaTime, engine::Application& app);
    void updateWorldSystems(float deltaTime, engine::Application& app);
    void updateFrameUniforms(engine::Renderer& renderer);
    void renderWorld(engine::Renderer& renderer);
    void renderPlayer(engine::Renderer& renderer);
};
```

## Scene Management

### Atmospheric Scene Runtime

The atmospheric scene runtime manages complex scene states:

```cpp
class AtmosphericSceneRuntime : public engine::RuntimeMode {
public:
    enum class SceneState {
        Intro,
        Exploration,
        Interaction,
        Transition
    };
    
    void initialize(engine::Application& app, engine::Renderer& renderer) override {
        // Initialize scene-specific assets
        loadSceneAssets(app);
        
        // Setup initial state
        currentState = SceneState::Intro;
        
        // Create scene entities
        createSceneEntities(app);
    }
    
    void update(float deltaTime, engine::Application& app, engine::Renderer& renderer) override {
        switch (currentState) {
            case SceneState::Intro:
                updateIntro(deltaTime, app);
                break;
            case SceneState::Exploration:
                updateExploration(deltaTime, app);
                break;
            case SceneState::Interaction:
                updateInteraction(deltaTime, app);
                break;
            case SceneState::Transition:
                updateTransition(deltaTime, app);
                break;
        }
    }
    
    void render(engine::Renderer& renderer) override {
        // Render based on current state
        switch (currentState) {
            case SceneState::Intro:
                renderIntro(renderer);
                break;
            case SceneState::Exploration:
                renderExploration(renderer);
                break;
            case SceneState::Interaction:
                renderInteraction(renderer);
                break;
            case SceneState::Transition:
                renderTransition(renderer);
                break;
        }
    }

private:
    SceneState currentState = SceneState::Intro;
    
    void loadSceneAssets(engine::Application& app);
    void createSceneEntities(engine::Application& app);
    
    // State-specific updates
    void updateIntro(float deltaTime, engine::Application& app);
    void updateExploration(float deltaTime, engine::Application& app);
    void updateInteraction(float deltaTime, engine::Application& app);
    void updateTransition(float deltaTime, engine::Application& app);
    
    // State-specific rendering
    void renderIntro(engine::Renderer& renderer);
    void renderExploration(engine::Renderer& renderer);
    void renderInteraction(engine::Renderer& renderer);
    void renderTransition(engine::Renderer& renderer);
};
```

## Metasset System

### Daylight Sandbox Scene

The metasset system manages complex scene data:

```cpp
class DaylightSandboxSceneMetasset {
public:
    struct SceneData {
        Vec3 cameraStartPosition;
        Vec3 cameraTarget;
        std::vector<SceneObject> objects;
        std::vector<Light> lights;
        PostProcessSettings postProcessSettings;
    };
    
    static std::shared_ptr<SceneData> load(const std::filesystem::path& path) {
        // Parse metasset file
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open metasset file");
        }
        
        auto data = std::make_shared<SceneData>();
        
        // Load scene configuration
        data->cameraStartPosition = loadVector3(file);
        data->cameraTarget = loadVector3(file);
        data->objects = loadObjects(file);
        data->lights = loadLights(file);
        data->postProcessSettings = loadPostProcessSettings(file);
        
        return data;
    }

private:
    static Vec3 loadVector3(std::ifstream& file) {
        Vec3 vec;
        file >> vec.x >> vec.y >> vec.z;
        return vec;
    }
    
    static std::vector<SceneObject> loadObjects(std::ifstream& file) {
        // Parse object list
        std::vector<SceneObject> objects;
        int count;
        file >> count;
        
        for (int i = 0; i < count; ++i) {
            SceneObject obj;
            obj.position = loadVector3(file);
            obj.rotation = loadVector3(file);
            obj.scale = loadVector3(file);
            std::getline(file, obj.meshPath);
            objects.push_back(obj);
        }
        
        return objects;
    }
};
```

## UI Overlay System

### Settings Overlay

The settings overlay provides user configuration:

```cpp
class SettingsOverlay {
public:
    struct Settings {
        // Graphics settings
        bool vsync = true;
        bool fullscreen = false;
        int resolutionWidth = 1280;
        int resolutionHeight = 720;
        
        // Audio settings
        float masterVolume = 1.0f;
        float musicVolume = 1.0f;
        float sfxVolume = 1.0f;
        
        // Game settings
        int difficulty = 1; // 0: Easy, 1: Normal, 2: Hard
        bool autoSave = true;
    };
    
    void initialize(engine::Application& app) {
        // Load settings from file
        loadSettings();
        
        // Setup UI elements
        setupControls(app);
    }
    
    void update(engine::Application& app, float deltaTime) {
        // Handle user input
        handleInput(app);
        
        // Update UI animations
        updateAnimations(deltaTime);
    }
    
    void render(engine::Renderer& renderer) {
        // Render settings panel
        renderPanel(renderer);
        
        // Render controls
        renderControls(renderer);
    }

private:
    Settings currentSettings;
    
    void loadSettings();
    void saveSettings();
    void setupControls(engine::Application& app);
    void handleInput(engine::Application& app);
    void updateAnimations(float deltaTime);
    void renderPanel(engine::Renderer& renderer);
    void renderControls(engine::Renderer& renderer);
};
```

## Building and Running

### Build Commands

```bash
# Configure and build the example
cmake --build build --target atmospheric_demo

# Run the example
./build/bin/atmospheric_demo
```

### Debugging

Enable debug features for development:

```cpp
// In main.cpp or initialization code
app.setDebugMode(true);
app.setShowFPS(true);
app.setShowProfiler(true);
```

## Current Limitations

- **3D Model Parsing**: Model assets are loaded as raw binary data only. Full parsing (vertices, faces, normals) is not yet implemented.
- **Character Models**: The example uses placeholder geometry until model parsing is complete.

## Key Features Demonstrated

1. **Runtime System**: Multiple game states with smooth transitions
2. **ECS Integration**: Entity management and component systems
3. **Asset Management**: Discovery, loading, and caching
4. **Advanced Rendering**: Shadows, post-processing, ray evaluation
5. **Camera System**: Free camera controls and view management
6. **UI Overlays**: Interactive settings and menu systems
7. **Scene Management**: Complex scene state handling
8. **Metasset System**: External configuration files

## Related Documentation

- [Runtime System](../runtime/overview.md) - Runtime architecture and registration
- [Renderer](../renderer/overview.md) - Graphics rendering system
- [ECS](../ecs/overview.md) - Entity Component System
- [Assets](../assets/overview.md) - Asset management
- [Quick Start](../quickstart.md) - Getting started guide
