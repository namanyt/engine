# Asset Management Overview

The asset management system handles resource discovery, loading, caching, and metadata generation for game assets. It provides a unified interface for managing different types of resources including textures, audio, models, and shaders.

## System Architecture

```
┌─────────────────────────────────────────────────────┐
│                     Application                     │
│                                                     │
│  • Runtime systems                                  │
│  • Entity management                                │
│  • Rendering pipeline                               │
└──────────────────────────┬──────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────┐
│                    Asset Manager                    │
│                                                     │
│  ┌────────────────────────────────────────────────┐ │
│  │                   Discovery                    │ │
│  │  • Directory scanning                          │ │
│  │  • File type detection                         │ │
│  │  • Asset registration                          │ │
│  └────────────────────────────────────────────────┘ │
│                                                     │
│  ┌────────────────────────────────────────────────┐ │
│  │                    Loading                     │ │
│  │  • Lazy loading                                │ │
│  │  • Caching                                     │ │
│  │  • Type-specific loaders                       │ │
│  └────────────────────────────────────────────────┘ │
│                                                     │
│  ┌────────────────────────────────────────────────┐ │
│  │                    Registry                    │ │
│  │  • Asset metadata                              │ │
│  │  • UUID management                             │ │
│  │  • Path resolution                             │ │
│  └────────────────────────────────────────────────┘ │
└──────────────────────────┬──────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────┐
│                  Asset Types                        │
│                                                     │
│  • Shader assets                                    │
│  • Texture assets                                   │
│  • Audio assets                                     │
│  • Model assets                                     │
│                                                     │
└──────────────────────────┬──────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────┐
│                   File System                       │
│                                                     │
│  • Disk I/O                                         │
│  • Format parsing                                   │
│  • Compression/decompression                        │
└─────────────────────────────────────────────────────┘
```

## Asset Discovery

### Automatic Discovery Process

The asset manager automatically discovers assets in specified directories:

```cpp
#include <engine/assets.hpp>

// Create asset manager
auto assetManager = std::make_shared<engine::AssetManager>();

// Discover assets in directory
std::size_t discoveredCount = assetManager->discover("assets");
std::cout << "Discovered " << discoveredCount << " assets" << std::endl;
```

### Supported Asset Types

The engine supports various asset types:

| Type     | Extensions              | Description                                   |
| -------- | ----------------------- | --------------------------------------------- |
| Shaders  | .glsl, .vert, .frag     | GLSL shader programs                          |
| Textures | .png, .jpg, .jpeg, .bmp | Image files                                   |
| Audio    | .wav, .ogg, .mp3        | Sound effects and music                       |
| Models   | .obj, .gltf             | 3D model files (OBJ parsed, glTF binary only) |

### Asset Metadata Generation

The system automatically generates metadata for discovered assets:

```cpp
// Example auto-generated metadata file: texture.png.meta
uuid=12345678-1234-5678-9abc-123456789012
asset_type=texture
tags=skybox, environment
preload=true
width=2048
height=1024
format=png
size_bytes=8388608
```

## Asset Loading

### Lazy Loading System

Assets are loaded on-demand to optimize memory usage:

```cpp
// Load asset by UUID
auto texture = assetManager->load<engine::TextureAsset>("texture-uuid");

// Load asset by path
auto audio = assetManager->load<engine::AudioAsset>("audio/music.ogg");

// Load asset using handle
auto model = assetManager->load(modelHandle);
```

### Asset Handles

Handles provide type-safe references to assets:

```cpp
// Get handle by UUID
auto handle = assetManager->findByUuid<engine::TextureAsset>(uuid);

// Get handle by path
auto handle = assetManager->findByPath<engine::AudioAsset>(assetPath);

// Check if handle is valid
if (handle) {
    // Load the asset
    auto asset = assetManager->load(handle);
}
```

### Loading States

Assets can be in different states:

```cpp
// Check if asset is loaded
bool isLoaded = assetManager->isLoaded(uuid);

// Reload asset from disk
bool success = assetManager->reload(uuid);

// Unload asset from memory
assetManager->unload(uuid);

// Unload all assets
assetManager->unloadAll();
```

## Asset Registry

### Registry Operations

The registry maintains metadata for all discovered assets:

```cpp
// Get registry reference
const engine::AssetRegistry& registry = assetManager->registry();

// Count assets by type
std::size_t textureCount = assetManager->count(engine::AssetType::Texture);
std::size_t audioCount = assetManager->count(engine::AssetType::Audio);

// Get total asset count
std::size_t totalCount = assetManager->totalCount();
```

### Path Resolution

The system handles asset path resolution:

```cpp
// Resolve relative paths to absolute paths
std::filesystem::path resolvedPath = assetManager->resolveAssetPath("textures/sky.png");

// Get asset root directory
const std::filesystem::path& rootDir = assetManager->assetRootDirectory();
```

## Asset Types

### Shader Assets

```cpp
struct ShaderAsset {
    enum class Stage {
        Vertex,
        Fragment
    };

    Stage stage() const;
    const std::string& source() const;
    std::filesystem::path filePath() const;
};

// Load and use shader asset
auto shader = assetManager->load<engine::ShaderAsset>("shaders/surface.vert");
if (shader) {
    // Create shader program
    auto program = std::make_unique<engine::Shader>(shader->filePath());
}
```

### Texture Assets

```cpp
struct TextureAsset {
    unsigned int textureId() const;
    int width() const;
    int height() const;
    std::string formatName() const;
    std::size_t sizeInBytes() const;
};

// Load and use texture asset
auto texture = assetManager->load<engine::TextureAsset>("textures/background.png");
if (texture) {
    // Use texture in rendering
    renderer.draw(mesh, material, transform, frameUniforms, texture->textureId());
}
```

### Audio Assets

```cpp
struct AudioAsset {
    unsigned int audioId() const;
    std::string formatName() const;
    std::size_t sizeInBytes() const;
    float duration() const;
};

// Load and use audio asset
auto audio = assetManager->load<engine::AudioAsset>("audio/music.ogg");
if (audio) {
    // Play audio
    audioSystem.play(audio->audioId());
}
```

### Model Assets

**Note:** Model assets are currently loaded as raw binary data. Full format parsing (vertices, faces, normals, etc.) is not yet implemented. This feature is planned for future development.

```cpp
struct ModelAsset {
    std::string formatName() const;       // Extension without dot (e.g., "obj")
    std::size_t sizeInBytes() const;      // File size in bytes
    const std::vector<std::uint8_t>& sourceBytes() const;  // Raw file data
};

// Load model asset (returns raw binary data)
auto model = assetManager->load<engine::ModelAsset>("models/character.obj");
if (model) {
    // Access raw model data
    auto bytes = model->sourceBytes();
    std::string format = model->formatName();
}
```

## Best Practices

### Asset Organization

1. **Directory Structure**: Organize assets by type and purpose
2. **Naming Conventions**: Use descriptive, consistent names
3. **Metadata Files**: Create metadata for important assets
4. **Version Control**: Track asset changes with source code

### Memory Management

1. **Lazy Loading**: Load assets on-demand rather than upfront
2. **Caching**: Leverage automatic caching for frequently used assets
3. **Preloading**: Preload critical assets during loading screens
4. **Unloading**: Unload unused assets to free memory

### Performance Optimization

1. **Asset Atlases**: Combine small textures into larger atlases
2. **Compression**: Use compressed formats where possible
3. **Level of Detail**: Provide multiple quality levels for assets
4. **Streaming**: Stream large assets rather than loading entirely

## Related Documentation

- [Discovery and Loading](discovery-and-loading.md) - Detailed asset management operations
- [Asset Types](asset-types.md) - Specific asset type information
- [Runtime Integration](../runtime/overview.md) - Using assets in game runtimes
