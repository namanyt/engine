#pragma once

#include "core/RuntimeOverlay.h"
#include "math/Transform.h"

#include <filesystem>
#include <memory>
#include <vector>

#include "core/RenderProfiler.h"
#include "world/RayTracing.h"
#include "world/Lighting.h"
#include "world/Material.h"

namespace engine
{
class AssetManager;
class Mesh;
class PostProcessor;
class RayEvaluationPass;
class Shader;
class ShaderLibrary;
class ShadowMapPass;

/// @brief Per-frame uniform data uploaded to shaders.
///
/// Aggregates camera matrices, lighting, fog, and post-processing parameters
/// into a single struct that is pushed to the GPU each frame.
struct FrameUniforms final
{
    Mat4 viewMatrix = Mat4::identity();              ///< Camera view matrix (world-to-view).
    Mat4 projectionMatrix = Mat4::identity();        ///< Camera projection matrix.
    Mat4 inverseViewMatrix = Mat4::identity();       ///< Inverse of the view matrix.
    Mat4 inverseProjectionMatrix = Mat4::identity(); ///< Inverse of the projection matrix.
    Mat4 previousInverseProjectionMatrix =
        Mat4::identity();                                 ///< Previous frame's inverse projection.
    Mat4 viewProjectionMatrix = Mat4::identity();         ///< Combined view * projection matrix.
    Mat4 previousViewProjectionMatrix = Mat4::identity(); ///< Previous frame's view-projection.
    Mat4 lightViewProjectionMatrix = Mat4::identity();    ///< Shadow-caster light VP matrix.
    Vec3 viewPosition{};                                  ///< Camera position in world space.
    Vec3 previousViewPosition{};                          ///< Previous frame's camera position.
    Vec3 viewForward{0.0f, 0.0f, -1.0f};                  ///< Camera forward direction.
    Vec3 previousViewForward{0.0f, 0.0f, -1.0f};          ///< Previous frame's forward direction.
    Vec3 viewRight{1.0f, 0.0f, 0.0f};                     ///< Camera right direction.
    Vec3 viewUp{0.0f, 1.0f, 0.0f};                        ///< Camera up direction.
    Vec3 previousLightDirection{0.0f, -1.0f, 0.0f};       ///< Previous frame's light direction.
    float timeSeconds = 0.0f;                             ///< Total elapsed time (seconds).
    int frameIndex = 0;       ///< Monotonically increasing frame counter.
    float aspectRatio = 1.0f; ///< Viewport width / height ratio.
    float verticalFieldOfViewRadians =
        0.78539816339f;                    ///< Vertical FOV in radians (default 45 deg).
    float nearPlane = 0.1f;                ///< Near clipping plane distance.
    Vec3 fogColor{0.18f, 0.24f, 0.30f};    ///< Fog tint color (RGB).
    float fogDensity = 0.02f;              ///< Exponential fog density factor.
    float fogBaseHeight = 3.0f;            ///< Height at which fog begins.
    float fogHeightFalloff = 0.08f;        ///< Vertical fog falloff rate.
    float fogMaxHeight = 96.0f;            ///< Maximum fog layer height.
    DirectionalLight directionalLight{};   ///< Primary directional (sun) light.
    std::vector<LocalLight> localLights{}; ///< Additional point/spot lights.
    ShadowSettings shadowSettings{};       ///< Shadow map resolution, bias, etc.
    SkyLight skyLight{};                   ///< Ambient/sky illumination parameters.
    RayEvaluationSettings rayEvaluation{}; ///< Ray-based evaluation settings.
    DebugViewSettings debugView{};         ///< Debug visualization mode.
    RayTracingScene rayTracingScene{};     ///< Ray tracing scene data.
    float exposure = 1.10f;                ///< Post-processing exposure multiplier.
    float bloomThreshold = 1.05f;          ///< Bloom bright-pass luminance threshold.
};

/// @brief Collection of internal render target texture IDs for debug inspection.
struct RendererDebugTextures final
{
    unsigned int shadowMapTextureId = 0;  ///< Current shadow map depth texture.
    unsigned int sceneDepthTextureId = 0; ///< Scene depth buffer texture.
    unsigned int volumetricTextureId = 0; ///< Volumetric lighting texture.
};

/**
 * @brief Core OpenGL renderer that owns draw submission and render state.
 *
 * The `Renderer` manages the viewport, shader library, post-processing,
 * shadow maps, and ray evaluation passes. It provides high-level `draw()`
 * calls that push frame uniforms, material state, and model transforms
 * to the GPU.
 *
 * @par Frame lifecycle
 * @code
 * renderer.beginFrame(clearColor);
 * // ... draw calls ...
 * renderer.endFrame(postSettings, uniforms, timeSeconds);
 * @endcode
 *
 * @see ShaderLibrary
 * @see PostProcessor
 * @see RenderPipeline
 */
class Renderer final
{
  public:
    /// @brief Constructs the renderer with an asset manager and shader directory.
    /// @param assetManager Shared pointer to the engine's AssetManager.
    /// @param shaderDirectory Base path for resolving shader files.
    Renderer(const std::shared_ptr<AssetManager>& assetManager,
             const std::filesystem::path& shaderDirectory);

    /// @brief Destroys all GPU resources owned by the renderer.
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    /// @brief Updates the OpenGL viewport to the new dimensions.
    /// @param width Viewport width in pixels.
    /// @param height Viewport height in pixels.
    void setViewport(int width, int height);

    /// @brief Prepares GPU resources required for overlay-only rendering.
    void prepareOverlayRenderingResources();

    /// @brief Prepares GPU resources required for full world rendering.
    void prepareWorldRenderingResources();

    /// @brief Begins a new frame; clears the framebuffer to the given color.
    /// @param clearColor RGBA clear color for the background.
    void beginFrame(const Color& clearColor);

    /// @brief Ends the current frame; applies post-processing and presents.
    /// @param postProcessSettings Bloom, tonemap, and other effect settings.
    /// @param frameUniforms Per-frame uniform data (matrices, lights, fog).
    /// @param timeSeconds Current application time in seconds.
    /// @param drawRuntimeOverlay Whether to composite the runtime overlay.
    void endFrame(const PostProcessSettings& postProcessSettings,
                  const FrameUniforms& frameUniforms, float timeSeconds,
                  bool drawRuntimeOverlay = true) const;

    /// @brief Ends an overlay-only render pass.
    void endOverlayFrame() const;

    /// @brief Copies the scene depth texture to the backbuffer (debug use).
    void restoreSceneDepthToBackbuffer() const;

    /// @brief Draws all configured runtime overlay layers.
    void drawRuntimeOverlayLayers() const;

    /// @brief Begins a shadow map rendering pass.
    /// @param frameUniforms Uniform data containing the light view-projection matrix.
    void beginShadowPass(const FrameUniforms& frameUniforms) const;

    /// @brief Submits a mesh for shadow-caster rendering using a raw model matrix.
    /// @param mesh GPU-ready mesh to draw.
    /// @param modelMatrix World-space transform of the mesh.
    void drawShadow(const Mesh& mesh, const Mat4& modelMatrix) const;

    /// @brief Submits a mesh for shadow-caster rendering using a Transform struct.
    /// @param mesh GPU-ready mesh to draw.
    /// @param transform World-space position, rotation, and scale.
    void drawShadow(const Mesh& mesh, const Transform& transform) const;

    /// @brief Ends the shadow map rendering pass.
    void endShadowPass() const;

    /// @brief Submits a mesh for geometry rendering with material and model matrix.
    /// @param mesh GPU-ready mesh to draw.
    /// @param material Surface properties (albedo, roughness, metallic, etc.).
    /// @param modelMatrix World-space transform of the mesh.
    /// @param frameUniforms Per-frame uniform data.
    /// @param textureId Optional texture override (0 = use material default).
    /// @param opacity Alpha multiplier for the draw call.
    void draw(const Mesh& mesh, const Material& material, const Mat4& modelMatrix,
              const FrameUniforms& frameUniforms, unsigned int textureId = 0,
              float opacity = 1.0f) const;

    /// @brief Submits a mesh for geometry rendering with material and Transform.
    /// @param mesh GPU-ready mesh to draw.
    /// @param material Surface properties (albedo, roughness, metallic, etc.).
    /// @param transform World-space position, rotation, and scale.
    /// @param frameUniforms Per-frame uniform data.
    /// @param textureId Optional texture override (0 = use material default).
    /// @param opacity Alpha multiplier for the draw call.
    void draw(const Mesh& mesh, const Material& material, const Transform& transform,
              const FrameUniforms& frameUniforms, unsigned int textureId = 0,
              float opacity = 1.0f) const;

    /// @brief Returns a mutable reference to the shader library.
    /// @return Reference to the internal ShaderLibrary.
    ShaderLibrary& shaderLibrary() noexcept;

    /// @brief Returns a const reference to the shader library.
    /// @return Const reference to the internal ShaderLibrary.
    const ShaderLibrary& shaderLibrary() const noexcept;

    /// @brief Returns a mutable reference to the render profiler.
    /// @return Reference to the internal RenderProfiler.
    RenderProfiler& profiler() noexcept;

    /// @brief Returns a const reference to the render profiler.
    /// @return Const reference to the internal RenderProfiler.
    const RenderProfiler& profiler() const noexcept;

    /// @brief Returns the current framebuffer width in pixels.
    /// @return Viewport width as an integer.
    int framebufferWidth() const noexcept;

    /// @brief Returns the current framebuffer height in pixels.
    /// @return Viewport height as an integer.
    int framebufferHeight() const noexcept;

    /// @brief Returns internal render target texture IDs for debug inspection.
    /// @return Struct containing shadow map, depth, and volumetric texture IDs.
    RendererDebugTextures debugTextures() const noexcept;

    /// @brief Sets the primary runtime overlay texture with default options.
    /// @param textureId OpenGL texture ID for the overlay.
    /// @param width Texture width in pixels.
    /// @param height Texture height in pixels.
    void setRuntimeOverlayTexture(unsigned int textureId, int width, int height);

    /// @brief Sets the primary runtime overlay texture with custom layout options.
    /// @param textureId OpenGL texture ID for the overlay.
    /// @param width Texture width in pixels.
    /// @param height Texture height in pixels.
    /// @param options Layout, opacity, and effect configuration.
    void setRuntimeOverlayTexture(unsigned int textureId, int width, int height,
                                  const RuntimeOverlayOptions& options);

    /// @brief Sets the secondary runtime overlay texture with default options.
    /// @param textureId OpenGL texture ID for the overlay.
    /// @param width Texture width in pixels.
    /// @param height Texture height in pixels.
    void setSecondaryRuntimeOverlayTexture(unsigned int textureId, int width, int height);

    /// @brief Sets the secondary runtime overlay texture with custom layout options.
    /// @param textureId OpenGL texture ID for the overlay.
    /// @param width Texture width in pixels.
    /// @param height Texture height in pixels.
    /// @param options Layout, opacity, and effect configuration.
    void setSecondaryRuntimeOverlayTexture(unsigned int textureId, int width, int height,
                                           const RuntimeOverlayOptions& options);

    /// @brief Clears the primary runtime overlay texture.
    void clearRuntimeOverlayTexture();

    /// @brief Clears the secondary runtime overlay texture.
    void clearSecondaryRuntimeOverlayTexture();

  private:
    void applyFrameState(const Shader& shader, const FrameUniforms& frameUniforms) const;
    void applyMaterialState(const Shader& shader, const Material& material) const;
    void applyLocalLightState(const Shader& shader, const FrameUniforms& frameUniforms) const;

    int m_framebufferWidth = 800;
    int m_framebufferHeight = 600;
    std::shared_ptr<ShaderLibrary> m_shaderLibrary;
    std::unique_ptr<PostProcessor> m_postProcessor;
    std::unique_ptr<RayEvaluationPass> m_rayEvaluationPass;
    std::unique_ptr<ShadowMapPass> m_shadowMapPass;
    mutable RenderProfiler m_profiler;
};
} // namespace engine
