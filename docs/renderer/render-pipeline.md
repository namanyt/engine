# Render Pipeline

The engine implements a multi-pass rendering pipeline that handles various stages of graphics processing, from shadow mapping to post-processing effects. Each pass builds upon the previous one to create the final rendered frame.

## Pipeline Overview

```
┌─────────────────────────────────────────────────────────┐
│                     Frame Start                         │
│  • Clear buffers                                        │
│  • Setup viewport                                       │
│  • Initialize state                                     │
└──────────────────────────────┬──────────────────────────┘
                               │
                               ▼
┌──────────────────────────────▼──────────────────────────┐
│                    Shadow Pass                          │
│                                                         │
│  • Render from light perspective                        │
│  • Generate depth map                                   │
│  • Apply shadow bias                                    │
│                                                         │
│  Output: Shadow Map Texture                             │
└──────────────────────────────┬──────────────────────────┘
                               │
                               ▼
┌──────────────────────────────▼──────────────────────────┐
│                    Scene Pass                           │
│                                                         │
│  • Render geometry                                      │
│  • Apply materials                                      │
│  • Compute lighting                                     │
│  • Generate scene depth                                 │
│                                                         │
│  Output: Color + Depth Textures                         │
└──────────────────────────────┬──────────────────────────┘
                               │
                               ▼
┌──────────────────────────────▼──────────────────────────┐
│                  Ray Evaluation Pass                    │
│                                                         │
│  • Trace rays from camera                               │
│  • Compute indirect lighting                            │
│  • Evaluate environment                                 │
│  • Generate volumetric effects                          │
│                                                         │
│  Output: Volumetric Texture                             │
└──────────────────────────────┬──────────────────────────┘
                               │
                               ▼
┌──────────────────────────────▼──────────────────────────┐
│                   Post-Processing Pass                  │
│                                                         │
│  • Bloom effect                                         │
│  • Tonemapping                                          │
│  • Color grading                                        │
│  • Anti-aliasing                                        │
│                                                         │
│  Output: Processed Frame                                │
└──────────────────────────────┬──────────────────────────┘
                               │
                               ▼
┌──────────────────────────────▼──────────────────────────┐
│                     Overlay Pass                        │
│                                                         │
│  • Draw UI elements                                     │
│  • Render debug information                             │
│  • Apply runtime overlays                               │
│                                                         │
│  Output: Final Frame                                    │
└──────────────────────────────┬──────────────────────────┘
                               │
                               ▼
┌──────────────────────────────▼──────────────────────────┐
│                     Frame End                           │
│                                                         │
│  • Present to screen                                    │
│  • Swap buffers                                         │
│  • Update profiler                                      │
└─────────────────────────────────────────────────────────┘
```

## Shadow Pass

### Purpose and Setup

The shadow pass renders the scene from the light's perspective to generate a depth map that determines which areas are in shadow.

```cpp
// Begin shadow pass
void beginShadowPass(const FrameUniforms& frameUniforms) {
    // Setup shadow framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

    // Set up light view/projection matrices
    Mat4 lightView = computeLightViewMatrix(frameUniforms.directionalLight.direction);
    Mat4 lightProjection = computeLightProjectionMatrix(
        frameUniforms.shadowSettings.radius,
        frameUniforms.shadowSettings.near,
        frameUniforms.shadowSettings.far
    );

    // Set viewport to shadow map size
    glViewport(0, 0, shadowMapWidth, shadowMapHeight);

    // Clear depth buffer
    glClear(GL_DEPTH_BUFFER_BIT);
}
```

### Shadow Rendering

```cpp
// Draw object for shadow map
void drawShadow(const Mesh& mesh, const Transform& transform) {
    // Compute model-view-projection matrix from light perspective
    Mat4 modelViewProjection =
        lightViewProjection * transform.computeModelMatrix();

    // Set shadow shader uniforms
    shadowShader->use();
    shadowShader->setMat4("u_mvp", modelViewProjection);

    // Draw mesh (only depth information matters)
    mesh.draw();
}
```

### Shadow Map Properties

```cpp
struct ShadowSettings {
    float radius = 50.0f;          // Light radius for shadow map
    float near = 1.0f;             // Near plane distance
    float far = 100.0f;            // Far plane distance
    int width = 2048;              // Shadow map width
    int height = 2048;             // Shadow map height
    float bias = 0.001f;           // Shadow bias to prevent artifacts
};
```

## Scene Pass

### Geometry Rendering

The scene pass renders all geometry with materials, lighting, and shadows applied.

```cpp
// Basic scene rendering
void renderScene(const std::vector<Entity>& entities) {
    // Sort entities by material for batching
    std::vector<Entity> sortedEntities = sortEntitiesByMaterial(entities);

    // Render each material batch
    Material currentMaterial;
    std::vector<Entity> currentBatch;

    for (const Entity& entity : sortedEntities) {
        const auto& material = getComponent<MaterialComponent>(entity).material;

        if (material != currentMaterial) {
            // Flush previous batch
            renderBatch(currentBatch, currentMaterial);

            // Start new batch
            currentMaterial = material;
            currentBatch.clear();
        }

        currentBatch.push_back(entity);
    }

    // Render final batch
    renderBatch(currentBatch, currentMaterial);
}
```

### Material Application

```cpp
// Apply material to shader
void applyMaterial(const Material& material) {
    surfaceShader->use();

    // Set diffuse color
    surfaceShader->setVec3("u_diffuseColor", material.diffuseColor);

    // Set specular properties
    surfaceShader->setFloat("u_specularStrength", material.specularStrength);
    surfaceShader->setFloat("u_shininess", material.shininess);

    // Bind texture if available
    if (material.textureId != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.textureId);
        surfaceShader->setBool("u_useTexture", true);
    } else {
        surfaceShader->setBool("u_useTexture", false);
    }
}
```

## Ray Evaluation Pass

### Purpose and Implementation

The ray evaluation pass traces rays from the camera to compute indirect lighting and volumetric effects.

```cpp
struct RayEvaluationSettings {
    bool enabled = true;
    int samplesPerPixel = 16;
    float maxDistance = 100.0f;
    float stepSize = 0.1f;
    int maxSteps = 100;
    float densityThreshold = 0.001f;
};

// Begin ray evaluation pass
void beginRayEvaluationPass(const FrameUniforms& frameUniforms) {
    if (!frameUniforms.rayEvaluation.enabled) return;

    // Bind volumetric framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, volumetricFBO);

    // Setup ray evaluation shader
    rayEvalShader->use();

    // Set ray evaluation uniforms
    rayEvalShader->setInt("u_samplesPerPixel", frameUniforms.rayEvaluation.samplesPerPixel);
    rayEvalShader->setFloat("u_maxDistance", frameUniforms.rayEvaluation.maxDistance);
    rayEvalShader->setFloat("u_densityThreshold", frameUniforms.rayEvaluation.densityThreshold);
}
```

## Post-Processing Pass

### Bloom Effect

The bloom effect creates a soft glow around bright areas of the image.

```cpp
struct BloomSettings {
    bool enabled = true;
    float threshold = 1.05f;      // Brightness threshold
    float intensity = 0.8f;       // Bloom intensity
    float radius = 1.5f;          // Blur radius
    int iterations = 4;           // Number of blur passes
};

// Apply bloom effect
void applyBloom(const PostProcessSettings& settings) {
    if (!settings.bloom.enabled) return;

    // Extract bright areas
    renderBrightPass(settings.bloom.threshold);

    // Apply Gaussian blur multiple times
    for (int i = 0; i < settings.bloom.iterations; ++i) {
        applyBlur(settings.bloom.radius * (i + 1));
    }

    // Composite bloom with original scene
    compositeBloom(settings.bloom.intensity);
}
```

### Tonemapping

Tonemapping adjusts the dynamic range of the image to fit within displayable values.

```cpp
struct TonemapSettings {
    bool enabled = true;
    float exposure = 1.10f;
    vec3 operatorColor = vec3(1.0f, 0.98f, 0.95f);
};

// Apply tonemapping
void applyTonemapping(const PostProcessSettings& settings) {
    if (!settings.tonemap.enabled) return;

    // Bind tonemap shader
    tonemapShader->use();

    // Set tonemap uniforms
    tonemapShader->setFloat("u_exposure", settings.tonemap.exposure);
    tonemapShader->setVec3("u_operatorColor", settings.tonemap.operatorColor);

    // Render tonemapped image to screen
    renderFullscreenQuad();
}
```

## Overlay Pass

### Purpose and Implementation

The overlay pass renders UI elements, debug information, and runtime-specific overlays on top of the scene.

```cpp
// Setup overlay rendering
void prepareOverlayRenderingResources() {
    // Create overlay framebuffer
    glGenFramebuffers(1, &overlayFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, overlayFBO);

    // Create overlay textures
    glGenTextures(2, overlayTextures);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, overlayTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
}

// Set runtime overlay texture
void setRuntimeOverlayTexture(unsigned int textureId, int width, int height) {
    overlayTextureId = textureId;
    overlayWidth = width;
    overlayHeight = height;
}
```

## Pipeline Configuration

### Enabling/Disabling Passes

```cpp
// Configure pipeline stages
RenderPipeline pipeline;

pipeline.enableShadowPass(true);           // Enable shadow mapping
pipeline.enableRayEvaluation(true);        // Enable ray evaluation
pipeline.enablePostProcessing(true);       // Enable post-processing
pipeline.enableOverlayRendering(true);     // Enable overlay rendering
```

### Performance Considerations

Each pass has different performance characteristics:

| Pass            | Performance Impact | Quality Impact | Recommended       |
| --------------- | ------------------ | -------------- | ----------------- |
| Shadow          | High               | High           | Always enable     |
| Scene           | Medium             | Critical       | Required          |
| Ray Evaluation  | Very High          | Medium         | Optional          |
| Post-Processing | Low-Medium         | Medium         | Enable in release |
| Overlay         | Low                | Low            | Enable for UI     |

## Best Practices

### Pipeline Optimization

1. **Profile Each Pass**: Use the profiler to identify bottlenecks
2. **Disable Unused Passes**: Turn off passes that aren't needed
3. **Adjust Quality Settings**: Lower shadow map resolution or ray samples
4. **Batch Operations**: Group similar rendering operations together

### Quality vs Performance Trade-offs

```cpp
// High quality settings (for release)
pipeline.enableAllPasses();
shadowSettings.width = 2048;
rayEvaluation.samplesPerPixel = 32;

// Balanced settings (for development)
pipeline.enableShadowPass(true);
pipeline.enableRayEvaluation(false);
shadowSettings.width = 1024;

// Low quality settings (for debugging)
pipeline.disableAllPasses();
pipeline.enableScenePass(true);
```

## Related Documentation

- [Renderer Overview](overview.md) - General renderer information
- [Shader Management](shader-management.md) - GLSL shader handling
- [Runtime Integration](../runtime/overview.md) - Using renderer in runtimes
