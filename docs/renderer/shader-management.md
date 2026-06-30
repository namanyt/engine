# Shader Management

The engine provides comprehensive shader management through the `Shader` class and `ShaderLibrary` system. Shaders are stored as external GLSL files and loaded at runtime, allowing for easy iteration without recompilation.

## Shader System Architecture

```
┌────────────────────────────────────────────────────┐
│                   Shader Library                   │
│                                                    │
│  • Path resolution                                 │
│  • Asset integration                               │
│  • Shader program cache                            │
│                                                    │
└──────────────────────┬─────────────────────────────┘
                       │
                       ▼
┌────────────────────────────────────────────────────┐
│                      Shader                        │
│                                                    │
│  • File loading                                    │
│  • Compilation                                     │
│  • Linking                                         │
│  • Uniform management                              │
│  • Validation                                      │
│                                                    │
└──────────────────────┬─────────────────────────────┘
                       │
                       ▼
┌────────────────────────────────────────────────────┐
│                   GLSL Files                       │
│                                                    │
│  • vertex.glsl                                     │
│  • fragment.glsl                                   │
│  • post_blur.vert/fag                              │
│  • ray_eval.vert/fag                               │
│  • shadow_depth.vert/fag                           │
│  • ui_overlay.frag                                 │
│                                                    │
└────────────────────────────────────────────────────┘
```

## Shader Class API

### Creating a Shader Program

```cpp
#include <engine/renderer.hpp>

// Create shader from file paths
std::filesystem::path vertexShader = "assets/shaders/vertex.glsl";
std::filesystem::path fragmentShader = "assets/shaders/fragment.glsl";

auto shader = std::make_unique<engine::Shader>(vertexShader, fragmentShader);
```

### Using Shader Programs

```cpp
// Bind shader program
shader->use();

// Set uniforms
shader->setMat4("u_projection", projectionMatrix);
shader->setMat4("u_view", viewMatrix);
shader->.setVec3("u_lightPos", lightPosition);
shader->setFloat("u_time", timeSeconds);

// Draw geometry
mesh.draw();

// Unbind shader (optional, happens automatically when new shader is bound)
shader->unbind();
```

### Uniform Setting Methods

The shader class provides methods for setting various uniform types:

```cpp
// Boolean uniforms
shader->setBool("u_enabled", true);
shader->setBoolv("u_flags", flagsArray, 4);

// Integer uniforms
shader->setInt("u_index", 0);
shader->setIntv("u_indices", indicesArray, 8);

// Float uniforms
shader->setFloat("u_value", 1.5f);
shader->setFloatv("u_values", valuesArray, 4);

// Vector uniforms
shader->setVec2("u_position", Vec2{10.0f, 20.0f});
shader->setVec3("u_color", Vec3{1.0f, 0.5f, 0.2f});
shader->setVec4("u_transform", Vec4{1.0f, 0.0f, 0.0f, 1.0f});

// Matrix uniforms
shader->setMat2("u_matrix2", Mat2::identity());
shader->setMat3("u_matrix3", Mat3::identity());
shader->setMat4("u_matrix4", Mat4::identity());

// Texture uniforms
shader->setTexture("u_texture", textureId, slot);
```

## Shader Library

### Initialization and Setup

```cpp
#include <engine/renderer.hpp>

// Create shader library with asset manager
std::shared_ptr<engine::AssetManager> assetManager =
    std::make_shared<engine::AssetManager>();

std::filesystem::path shaderDirectory = "assets/shaders";

auto shaderLibrary = std::make_unique<engine::ShaderLibrary>(
    assetManager,
    shaderDirectory
);
```

### Loading Shaders

```cpp
// Load shader by name (automatically finds .vert and .frag files)
auto shader = shaderLibrary->load("surface");

// Get shader path for direct access
std::filesystem::path vertexPath = shaderLibrary->shaderPath("surface.vert");
std::filesystem::path fragmentPath = shaderLibrary->shaderPath("surface.frag");
```

### Shader File Organization

Recommended directory structure:

```
assets/shaders/
├── surface.vert          # Surface lighting vertex shader
├── surface.frag          # Surface lighting fragment shader
├── post_blur.vert        # Post-processing blur vertex shader
├── post_blur.frag        # Post-processing blur fragment shader
├── ray_eval.vert         # Ray evaluation vertex shader
├── ray_eval.frag         # Ray evaluation fragment shader
├── shadow_depth.vert     # Shadow depth vertex shader
├── shadow_depth.frag     # Shadow depth fragment shader
└── ui_overlay.frag       # UI overlay fragment shader
```

## GLSL Shader Examples

### Basic Surface Vertex Shader

```glsl
#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main() {
    // Transform position to world space
    vec4 worldPos = u_model * vec4(aPosition, 1.0);

    // Transform to view and projection space
    gl_Position = u_projection * u_view * worldPos;

    // Pass data to fragment shader
    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(u_model))) * aNormal;
    TexCoords = aTexCoords;
}
```

### Basic Surface Fragment Shader

```glsl
#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform vec3 u_viewPos;

uniform vec3 u_diffuseColor;
uniform float u_specularStrength;
uniform float u_shininess;

uniform sampler2D u_texture;
uniform bool u_useTexture;

out vec4 FragColor;

void main() {
    // Ambient lighting
    vec3 ambient = vec3(0.1) * u_diffuseColor;

    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(u_lightDir);
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diff * u_lightColor * u_diffuseColor;

    // Specular lighting
    vec3 viewDir = normalize(u_viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_shininess);
    vec3 specular = spec * u_specularStrength * u_lightColor;

    // Combine lighting
    vec3 color = ambient + diffuse + specular;

    // Apply texture if enabled
    if (u_useTexture && texture(u_texture, TexCoords).a > 0.0) {
        color *= vec4(texture(u_texture, TexCoords));
    }

    FragColor = vec4(color, 1.0);
}
```

## Shader Validation and Error Handling

### Compile Status Checking

The engine automatically validates shader compilation:

```cpp
try {
    auto shader = std::make_unique<engine::Shader>(vertexPath, fragmentPath);
    // Shader compiled and linked successfully
} catch (const std::runtime_error& e) {
    // Handle compilation or linking errors
    std::cerr << "Shader error: " << e.what() << std::endl;
}
```

### Runtime Validation

```cpp
// Check shader program validity
if (!shader->isValid()) {
    std::cerr << "Invalid shader program" << std::endl;
}

// Get compile info log
std::InfoLog infoLog = shader->getInfoLog();
if (!infoLog.compileSuccess) {
    std::cerr << "Compile failed: " << infoLog.compileLog << std::endl;
}
```

## Best Practices

### Shader Organization

1. **Use Separate Files**: Keep vertex and fragment shaders in separate files
2. **Consistent Naming**: Use descriptive names that indicate purpose
3. **Modular Design**: Break complex shaders into smaller, reusable components
4. **Version Control**: Track shader changes alongside code changes

### Performance Optimization

1. **Minimize Instructions**: Reduce arithmetic operations in fragment shaders
2. **Use Texture Lookups**: Replace complex calculations with texture sampling
3. **Batch by Shader**: Group objects that use the same shader
4. **Avoid Branching**: Minimize if statements in shader code

### Debugging

1. **Visual Debugging**: Use color outputs to identify rendering issues
2. **Shader Profiling**: Profile shader performance on target hardware
3. **Error Checking**: Always validate shader compilation and linking
4. **Incremental Changes**: Make small, testable changes to shaders

## Related Documentation

- [Renderer Overview](overview.md) - General renderer information
- [Render Pipeline](render-pipeline.md) - Multi-pass rendering
- [Asset Management](../assets/overview.md) - Shader asset handling
