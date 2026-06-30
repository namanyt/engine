# Asset Discovery and Loading

This document provides detailed information about the asset discovery process, loading mechanisms, and cache management in the engine's asset system.

## Discovery Process

### Directory Scanning

The asset manager scans directories recursively to find supported assets:

```cpp
// Discover assets in a directory
std::size_t count = assetManager->discover("assets");

// Discover assets with specific filters
std::vector<std::string> extensions = {".png", ".jpg", ".wav", ".ogg"};
std::size_t filteredCount = assetManager->discover("assets", extensions);
```

### File Type Detection

The system automatically detects asset types based on file extensions:

```cpp
// Supported extensions by type
struct AssetTypeExtensions {
    static const std::vector<std::string> getSupportedExtensions(AssetType type) {
        switch (type) {
            case AssetType::Shader:
                return {".glsl", ".vert", ".frag"};
            case AssetType::Texture:
                return {".png", ".jpg", ".jpeg", ".bmp", ".tga"};
            case AssetType::Audio:
                return {".wav", ".ogg", ".mp3", ".flac"};
            case AssetType::Model:
                return {".obj", ".fbx", ".gltf"};
            default:
                return {};
        }
    }
};
```

### Asset Registration

Discovered assets are automatically registered with metadata:

```cpp
// Manual asset registration
engine::AssetHandle<> handle = assetManager->registerAsset("custom/path/asset.png");

// Automatic registration during discovery
// Assets are registered as they're found in the directory scan
```

## Loading Mechanisms

### Lazy Loading

Assets are loaded on-demand to optimize memory usage:

```cpp
// Asset loading flow:
// 1. Request load by UUID or path
// 2. Check cache for existing instance
// 3. If not cached, load from disk
// 4. Create asset instance
// 5. Cache the instance
// 6. Return reference to caller

auto texture = assetManager->load<engine::TextureAsset>(uuid);
```

### Loading States

Assets can be in different states during their lifecycle:

```cpp
enum class AssetLoadState {
    Unloaded,      // Not yet loaded into memory
    Loading,       // Currently being loaded from disk
    Loaded,        // Successfully loaded and cached
    Failed         // Loading failed (file not found, format error)
};

// Check loading state
bool isLoaded = assetManager->isLoaded(uuid);
```

### Preloading

Critical assets can be preloaded during startup:

```cpp
// Preload configuration in metadata file:
// preload=true

// Automatic preloading during discovery
std::size_t count = assetManager->discover("assets");
// Assets with preload=true are automatically loaded

// Manual preloading
auto criticalAssets = {
    "textures/ui.png",
    "audio/menu_music.ogg",
    "shaders/ui_overlay.frag"
};

for (const auto& path : criticalAssets) {
    assetManager->load<engine::TextureAsset>(path);
}
```

## Cache Management

### Automatic Caching

The asset manager maintains a cache of loaded assets:

```cpp
// Cache operations are automatic
auto texture1 = assetManager->load<engine::TextureAsset>(uuid);  // Loads and caches
auto texture2 = assetManager->load<engine::TextureAsset>(uuid);  // Returns cached instance

// Both references point to the same asset instance
assert(texture1 == texture2);
```

### Cache Control

Manual cache management for memory optimization:

```cpp
// Reload asset from disk (bypasses cache)
bool success = assetManager->reload(uuid);

// Unload specific asset
assetManager->unload(uuid);

// Unload all assets
assetManager->unloadAll();

// Check if asset is in cache
bool isCached = assetManager->isLoaded(uuid);
```

### Memory Tracking

Monitor asset memory usage:

```cpp
// Get total asset count
std::size_t totalCount = assetManager->totalCount();

// Count assets by type
std::size_t textureCount = assetManager->count(engine::AssetType::Texture);
std::size_t audioCount = assetManager->count(engine::AssetType::Audio);

// Estimate memory usage (platform-dependent)
std::size_t estimatedMemoryUsage = estimateMemoryUsage(assetManager);
```

## Advanced Loading Features

### Custom Loaders

Register custom loaders for specific asset types:

```cpp
// Define custom loader function
std::shared_ptr<engine::Asset> customLoader(const engine::AssetRegistry::Record& record) {
    // Implement custom loading logic
    auto customAsset = std::make_shared<CustomAsset>();
    
    // Load from file
    std::ifstream file(record.meta.filePath, std::ios::binary);
    if (!file.is_open()) {
        return nullptr;
    }
    
    // Parse custom format
    customAsset->parse(file);
    
    return customAsset;
}

// Register custom loader
assetManager->registerLoader(AssetType::Custom, customLoader);
```

### Async Loading

Load assets on background threads:

```cpp
// Async loading wrapper
class AsyncAssetLoader {
public:
    template<typename T>
    std::future<std::shared_ptr<T>> loadAsync(const std::string& uuid) {
        return std::async([uuid, this]() -> std::shared_ptr<T> {
            return assetManager->load<T>(uuid);
        });
    }

private:
    std::shared_ptr<engine::AssetManager> assetManager;
};

// Usage
AsyncAssetLoader asyncLoader(assetManager);
auto future = asyncLoader.loadAsync<engine::TextureAsset>("texture-uuid");

// Get result when ready
auto texture = future.get();
```

### Loading Progress

Track loading progress for large assets:

```cpp
// Progress callback structure
struct LoadingProgress {
    std::string assetName;
    float progress;  // 0.0 to 1.0
    bool completed;
};

// Progress tracking wrapper
class ProgressTracker {
public:
    void setCallback(std::function<void(const LoadingProgress&)> callback) {
        m_callback = std::move(callback);
    }
    
    template<typename T>
    std::shared_ptr<T> loadWithProgress(const std::string& uuid) {
        // Report start
        LoadingProgress progress;
        progress.assetName = "Loading asset...";
        progress.progress = 0.0f;
        progress.completed = false;
        if (m_callback) m_callback(progress);
        
        // Load asset
        auto asset = assetManager->load<T>(uuid);
        
        // Report completion
        progress.progress = 1.0f;
        progress.completed = true;
        if (m_callback) m_callback(progress);
        
        return asset;
    }

private:
    std::function<void(const LoadingProgress&)> m_callback;
};
```

## Error Handling

### Loading Errors

Handle common loading scenarios:

```cpp
// File not found
try {
    auto texture = assetManager->load<engine::TextureAsset>("nonexistent.png");
} catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "File not found: " << e.what() << std::endl;
}

// Format error
try {
    auto audio = assetManager->load<engine::AudioAsset>("invalid_file.wav");
} catch (const std::runtime_error& e) {
    std::cerr << "Format error: " << e.what() << std::endl;
}

// Invalid UUID
auto handle = assetManager->findByUuid<engine::TextureAsset>(uuid);
if (!handle) {
    std::cerr << "Invalid UUID: " << uuid << std::endl;
}
```

### Fallback Assets

Provide fallback assets for failed loads:

```cpp
template<typename T>
std::shared_ptr<T> loadWithFallback(
    const std::string& primaryPath, 
    const std::string& fallbackPath) {
    
    try {
        return assetManager->load<T>(primaryPath);
    } catch (...) {
        std::cerr << "Primary asset failed, using fallback: " << fallbackPath << std::endl;
        return assetManager->load<T>(fallbackPath);
    }
}
```

## Best Practices

### Discovery Optimization

1. **Discover Once**: Call discovery during initialization, not every frame
2. **Use Specific Directories**: Target specific directories rather than entire asset tree
3. **Cache Results**: Store discovered asset counts for monitoring
4. **Handle Errors Gracefully**: Log errors but continue loading process

### Loading Optimization

1. **Preload Critical Assets**: Load essential assets during startup/loading screens
2. **Stream Large Assets**: Use streaming for very large files
3. **Batch Loads**: Group load requests when possible
4. **Monitor Memory**: Track memory usage and unload unused assets

### Cache Management

1. **Use Automatic Caching**: Leverage built-in caching for frequently used assets
2. **Manual Unloading**: Unload assets that won't be needed again
3. **Reload When Needed**: Reload assets that change during development
4. **Clear Cache Strategically**: Clear cache during scene transitions

## Related Documentation

- [Asset Overview](overview.md) - General asset management concepts
- [Asset Types](asset-types.md) - Specific asset type information
- [Runtime Integration](../runtime/overview.md) - Using assets in game runtimes
