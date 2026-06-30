# Renderer Overview

The renderer is responsible for all graphics operations in the engine, including frame management, shader handling, draw submission, and post-processing effects. It provides a high-level interface to OpenGL 3.3 Core Profile while maintaining performance and flexibility.

## System Architecture

```
┌─────────────────────────────────────────────────────┐
│                    Application                      │
│  • Window management                                │
│  • Input handling                                   │
└──────────────────────────┬──────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────┐
│                     Renderer                        │
│                                                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │   Frame     │  │    Shader   │  │   Draw      │  │
│  │   Management│  │   Library   │  │   System    │  │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  │
│         │                │                │         │
│  ┌──────▼──────┐  ┌──────▼──────┐  ┌──────▼──────┐  │
│  │ Post-       │  │   Shadow    │  │   Ray       │  │
│  │ Processing  │  │    Map      │  │ Evaluation  │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  │
│                                                     │
│  ┌───────────────────────────────────────────────┐  │
│  │                    Profiler                   │  │
│  │  • Frame timing                               │  │
│  │  • Performance metrics                        │  │
│  └───────────────────────────────────────────────┘  │
└──────────────────────────┬──────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────┐
│                      OpenGL 3.3                     │
│  • VAO/VBO/EBO                                      │
│  • Textures                                         │
│  • Framebuffers                                     │
│  • Shaders                                          │
└─────────────────────────────────────────────────────┘
```

## Renderer Initialization

### Creating a Renderer Instance

```cpp
#include <engine/renderer.hpp>
#include <filesystem>

// Create renderer with asset manager and shader path
std::shared_ptr<engine::AssetManager> assetManager =
    std::make_shared<engine::AssetManager>();

std::filesystem::path shaderDirectory = "assets/shaders";

auto renderer = std::make_unique<engine::Renderer>(
    assetManager,
    shaderDirectory
);
```

### Setting Viewport

```cpp
// Set viewport when window is resized
renderer->setViewport(width, height);
```

## Frame Management

### Basic Frame Structure

```cpp
// Begin frame with clear color
Color clearColor{0.1f, 0.2f, 0.3f}; // Dark blue background
renderer->beginFrame(clearColor);

// Draw scene objects
drawScene(renderer);

// End frame with post-processing
PostProcessSettings postProcessSettings;
postProcessSettings.bloom.enabled = true;
postProcessSettings.bloom.threshold = 1.05f;

FrameUniforms frameUniforms;
updateFrameUniforms(frameUniforms, currentTransform);

renderer->endFrame(
    postProcessSettings,
    frameUniforms,
    timeSeconds,
    true  // Draw runtime overlay
);
```

### Frame Uniforms

Frame uniforms contain per-frame data passed to shaders:

```cpp
struct FrameUniforms final {
    // Camera matrices
    Mat4 viewMatrix = Mat4::identity();
    Mat4 projectionMatrix = Mat4::identity();
    Mat4 viewProjectionMatrix = Mat4::identity();

    // Camera state
    Vec3 viewPosition{};
    Vec3 viewForward{0.0f, 0.0f, -1.0f};
    Vec3 viewRight{1.0f, 0.0f, 0.0f};
    Vec3 viewUp{0.0f, 1.0f, 0.0f};

    // Timing
    float timeSeconds = 0.0f;
    int frameIndex = 0;

    // Environment
    float aspectRatio = 1.0f;
    Vec3 fogColor{0.18f, 0.24f, 0.30f};
    float fogDensity = 0.02f;

    // Lighting
    DirectionalLight directionalLight{};
    std::vector<LocalLight> localLights{};

    // Shadow settings
    ShadowSettings shadowSettings{};

    // Ray evaluation
    RayEvaluationSettings rayEvaluation{};

    // Debug view
    DebugViewSettings debugView{};
};
```

## Draw System

### Basic Drawing

```cpp
// Draw a mesh with material and transform
Material material;
material.diffuseColor = Color{1.0f, 0.5f, 0.2f};
material.specularStrength = 0.3f;

Transform transform;
transform.position = Vec3{0.0f, 0.0f, 0.0f};
transform.scale = Vec3{1.0f, 1.0f, 1.0f};

// Note: Use procedural geometry or custom meshes for now
// Model file parsing is not yet implemented
renderer->draw(
    mesh,
    material,
    transform,
    frameUniforms,
    textureId,  // Optional texture ID
    1.0f        // Opacity (1.0 = fully opaque)
);
```

### Drawing with Model Matrix

```cpp
// Draw using pre-computed model matrix
Mat4 modelMatrix = transform.computeModelMatrix();

renderer->draw(
    mesh,
    material,
    modelMatrix,
    frameUniforms,
    textureId,
    opacity
);
```

## Shadow Mapping

### Shadow Pass Setup

```cpp
// Begin shadow pass
renderer->beginShadowPass(frameUniforms);

// Draw scene for shadow map
for (const auto& entity : entities) {
    const auto& mesh = getComponent<MeshComponent>(entity).mesh;
    const auto& transform = getComponent<TransformComponent>(entity).transform;

    renderer->drawShadow(mesh, transform);
}

// End shadow pass
renderer->endShadowPass();
```

## Post-Processing

### Bloom Effect

```cpp
PostProcessSettings postProcessSettings;

// Enable bloom
postProcessSettings.bloom.enabled = true;
postProcessSettings.bloom.threshold = 1.05f;  // Brightness threshold
postProcessSettings.bloom.intensity = 0.8f;   // Bloom intensity
postProcessSettings.bloom.radius = 1.5f;      // Blur radius

// Apply in endFrame call
renderer->endFrame(postProcessSettings, frameUniforms, timeSeconds);
```

### Tonemapping

```cpp
// Configure tonemapping
postProcessSettings.tonemap.enabled = true;
postProcessSettings.tonemap.exposure = 1.10f;
postProcessSettings.tonemap.operatorColor = Color{1.0f, 0.98f, 0.95f};
```

## Render Pipeline

The renderer uses a multi-pass pipeline:

1. **Shadow Pass**: Generates shadow maps
2. **Scene Pass**: Renders main scene geometry
3. **Ray Evaluation Pass**: Computes ray-traced lighting
4. **Post-Processing Pass**: Applies bloom, tonemapping, etc.
5. **Overlay Pass**: Draws UI and overlay elements

### Pipeline Configuration

```cpp
// Configure pipeline stages
RenderPipeline pipeline;
pipeline.enableShadowPass(true);
pipeline.enableRayEvaluation(true);
pipeline.enablePostProcessing(true);
```

## Performance Profiling

### Accessing the Profiler

```cpp
// Get profiler reference
engine::RenderProfiler& profiler = renderer->profiler();

// Profile specific sections
profiler.beginSection("Scene Rendering");
// ... rendering code ...
profiler.endSection();

// Get timing information
auto timings = profiler.getTimings();
std::cout << "Shadow pass: " << timings.shadowPass << "ms\n";
std::cout << "Scene pass: " << timings.scenePass << "ms\n";
```

## Shader Management

The renderer integrates with the shader library for managing GLSL shaders:

```cpp
// Access shader library
engine::ShaderLibrary& shaderLib = renderer->shaderLibrary();

// Load shader program
auto shader = shaderLib.load("surface");

// Get shader path
std::filesystem::path path = shaderLib.shaderPath("surface.vert");
```

## Best Practices

### Performance Optimization

1. **Batch Draw Calls**: Group objects by material and shader
2. **Minimize State Changes**: Sort rendering by shader program
3. **Use VAOs**: Leverage vertex array objects for efficient rendering
4. **Profile Regularly**: Use the built-in profiler to identify bottlenecks

### Memory Management

1. **Reuse Buffers**: Keep vertex/index buffers alive between frames
2. **Texture Atlases**: Combine small textures into larger atlases
3. **Level of Detail**: Use appropriate mesh complexity for distance

### Code Organization

1. **Separate Setup and Update**: Keep initialization code separate from per-frame updates
2. **Use Components**: Store rendering data in ECS components
3. **Document Materials**: Clearly define material properties and usage

## Related Documentation

- [Shader Management](shader-management.md) - Working with GLSL shaders
- [Render Pipeline](render-pipeline.md) - Multi-pass rendering details
- [Runtime Integration](../runtime/overview.md) - Using renderer in runtimes
