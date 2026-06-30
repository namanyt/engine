# Asset Types

This document provides detailed information about each asset type supported by the engine, including their properties, usage patterns, and specific considerations.

## Asset Type System

### Base Asset Class

All asset types inherit from the base `Asset` class:

```cpp
class Asset {
public:
    virtual ~Asset() = default;
    
    // Common properties for all assets
    std::string uuid() const;
    std::filesystem::path filePath() const;
    std::size_t sizeInBytes() const;
    AssetType type() const;
    
    // Loading state management
    bool isLoaded() const;
    virtual void unload();
};

// Asset type enumeration
enum class AssetType : uint32_t {
    Unknown = 0,
    Shader = 1,
    Texture = 2,
    Audio = 3,
    Model = 4,
    Custom = 100
};
```

## Shader Assets

### Overview

Shader assets manage GLSL shader source code and compilation:

```cpp
class ShaderAsset : public Asset {
public:
    enum class Stage {
        Vertex,
        Fragment
    };
    
    // Shader-specific properties
    Stage stage() const;
    const std::string& source() const;
    bool isCompiled() const;
    
    // Compilation information
    std::string getCompileLog() const;
    bool hasCompilationErrors() const;
};
```

### Usage Example

```cpp
// Load shader asset
auto vertexShader = assetManager->load<engine::ShaderAsset>("shaders/surface.vert");

if (vertexShader) {
    // Verify shader stage
    assert(vertexShader->stage() == engine::ShaderAsset::Stage::Vertex);
    
    // Get source code
    const std::string& source = vertexShader->source();
    
    // Create shader program
    auto fragmentShader = assetManager->load<engine::ShaderAsset>("shaders/surface.frag");
    auto program = std::make_unique<engine::Shader>(
        vertexShader->filePath(),
        fragmentShader->filePath()
    );
}
```

### Shader File Structure

Recommended shader file organization:

```
assets/shaders/
├── surface.vert          # Surface lighting vertex shader
├── surface.frag          # Surface lighting fragment shader
├── post_blur.vert        # Post-processing blur vertex shader
├── post_blur.frag        # Post-processing blur fragment shader
├── ray_eval.vert         # Ray evaluation vertex shader
├── ray_eval.frag         # Ray evaluation fragment shader
└── shadow_depth.vert     # Shadow depth vertex shader
```

## Texture Assets

### Overview

Texture assets manage image files and OpenGL texture objects:

```cpp
class TextureAsset : public Asset {
public:
    // Texture properties
    unsigned int textureId() const;
    int width() const;
    int height() const;
    std::string formatName() const;  // "png", "jpg", etc.
    
    // Texture settings
    bool isSRGB() const;
    bool isMipmapped() const;
    TextureWrapMode wrapS() const;
    TextureWrapMode wrapT() const;
};

enum class TextureWrapMode {
    Repeat,
    ClampToEdge,
    MirrorRepeat,
    MirrorClampToEdge
};
```

### Usage Example

```cpp
// Load texture asset
auto texture = assetManager->load<engine::TextureAsset>("textures/background.png");

if (texture) {
    // Get texture ID for rendering
    unsigned int textureId = texture->textureId();
    
    // Check texture dimensions
    std::cout << "Texture size: " << texture->width() << "x" 
              << texture->height() << std::endl;
    
    // Use in rendering
    renderer.draw(mesh, material, transform, frameUniforms, textureId);
}
```

### Texture Format Support

| Format | Supported | sRGB Default | Mipmapping |
|--------|-----------|--------------|------------|
| PNG | Yes | Yes | Yes |
| JPEG | Yes | Yes | Yes |
| BMP | Yes | No | Yes |
| TGA | Yes | Yes | Yes |

## Audio Assets

### Overview

Audio assets manage sound effects and music files:

```cpp
class AudioAsset : public Asset {
public:
    // Audio properties
    unsigned int audioId() const;
    std::string formatName() const;  // "wav", "ogg", etc.
    float duration() const;           // Duration in seconds
    int sampleRate() const;
    int channels() const;
    
    // Playback state
    bool isPlaying() const;
    float currentTime() const;
};
```

### Usage Example

```cpp
// Load audio asset
auto music = assetManager->load<engine::AudioAsset>("audio/menu_music.ogg");

if (music) {
    // Get audio ID for playback
    unsigned int audioId = music->audioId();
    
    // Check duration
    std::cout << "Music duration: " << music->duration() << " seconds" << std::endl;
    
    // Play audio
    audioSystem.play(audioId);
}
```

### Audio Format Support

| Format | Supported | Compression | Quality |
|--------|-----------|-------------|---------|
| WAV | Yes | Uncompressed | Lossless |
| OGG | Yes | Compressed | Good |
| MP3 | Yes | Compressed | Fair |
| FLAC | Yes | Compressed | Lossless |

## Model Assets

### Overview

Model assets manage 3D model files by loading them as raw binary data. Full format parsing is not yet implemented.

```cpp
class ModelAsset : public Asset {
public:
    // Model properties
    std::string formatName() const;  // Extension without dot (e.g., "obj")
    std::size_t sizeInBytes() const;  // File size in bytes
    
    // Raw model data
    const std::vector<std::uint8_t>& sourceBytes() const;
};
```

### Usage Example

```cpp
// Load model asset (returns raw binary data)
auto character = assetManager->load<engine::ModelAsset>("models/character.obj");

if (character) {
    // Access raw model data
    auto bytes = character->sourceBytes();
    std::string format = character->formatName();
    
    std::cout << "Model size: " << character->sizeInBytes() << " bytes" << std::endl;
}
```

### Model Format Support

| Format | Supported | Features |
|--------|-----------|----------|
| OBJ | Binary only | Raw file loading (no parsing) |
| glTF | Binary only | Raw file loading (no parsing) |
| FBX | Not supported | Planned for future |

**Current Limitations:**
- Files are loaded as raw binary data
- No vertex/face/normal parsing
- No animation support
- No material extraction

## Custom Asset Types

### Creating Custom Assets

```cpp
// Define custom asset type
class CustomAsset : public engine::Asset {
public:
    // Custom properties
    std::vector<float> data;
    int customValue = 0;
    
    // Custom methods
    bool parse(std::ifstream& file) {
        // Implement parsing logic
        return true;
    }
};

// Register custom asset type
engine::AssetType kCustomAssetType = static_cast<engine::AssetType>(
    engine::AssetType::Custom + 1
);

// Create custom loader
std::shared_ptr<engine::Asset> customLoader(
    const engine::AssetRegistry::Record& record) {
    
    auto customAsset = std::make_shared<CustomAsset>();
    
    std::ifstream file(record.meta.filePath, std::ios::binary);
    if (!file.is_open()) {
        return nullptr;
    }
    
    customAsset->parse(file);
    return customAsset;
}

// Register loader with asset manager
assetManager->registerLoader(kCustomAssetType, customLoader);
```

## Asset Metadata

### Automatic Metadata Generation

The system generates metadata files for discovered assets:

```
# Example texture.png.meta
uuid=12345678-1234-5678-9abc-123456789012
asset_type=texture
tags=skybox, environment
preload=true
width=2048
height=1024
format=png
size_bytes=8388608
mipmapped=true
srgb=true

# Example music.ogg.meta
uuid=87654321-4321-8765-cba9-876543210987
asset_type=audio
tags=music, menu
preload=true
duration=180.5
sample_rate=44100
channels=2
format=ogg
```

### Manual Metadata Configuration

Create custom metadata files for specific assets:

```
# custom_asset.meta
uuid=custom-uuid-here
asset_type=texture
tags=ui, buttons
preload=false
width=512
height=256
format=png
mipmapped=false
srgb=true
wrap_s=clamp_to_edge
wrap_t=clamp_to_edge
```

## Best Practices

### Asset Optimization

1. **Texture Atlases**: Combine small textures into larger atlases
2. **Audio Compression**: Use compressed formats for music and effects
3. **Model LOD**: Provide multiple quality levels for models
4. **Shader Optimization**: Minimize shader instructions and branching

### Memory Management

1. **Lazy Loading**: Load assets on-demand
2. **Preloading**: Preload critical assets during loading screens
3. **Unloading**: Unload unused assets to free memory
4. **Caching**: Leverage automatic caching for frequently used assets

### Organization

1. **Directory Structure**: Organize by type and purpose
2. **Naming Conventions**: Use descriptive, consistent names
3. **Metadata Files**: Create metadata for important assets
4. **Version Control**: Track asset changes with source code

## Related Documentation

- [Asset Overview](overview.md) - General asset management concepts
- [Discovery and Loading](discovery-and-loading.md) - Asset loading operations
- [Runtime Integration](../runtime/overview.md) - Using assets in game runtimes
