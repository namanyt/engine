#pragma once

#include "core/FullScreenPass.h"
#include "core/RuntimeOverlay.h"
#include "core/ShaderLibrary.h"
#include "world/Lighting.h"

#include <memory>

namespace engine
{
class Shader;

/**
 * @brief Post-processing pipeline for scene rendering.
 *
 * Manages framebuffers, textures, and full-screen passes for:
 * - Scene color and depth buffering
 * - Bloom extraction and blur (ping-pong technique)
 * - Tonemapping and final composition
 * - Runtime overlay compositing
 *
 * Call `beginScene()` before geometry rendering, then `endScene()` to
 * apply post-processing effects and present the result.
 *
 * @see Renderer
 * @see FullScreenPass
 */
class PostProcessor final
{
  public:
    /// @brief Constructs a PostProcessor with the given shader library.
    /// @param shaderLibrary Shared pointer to the ShaderLibrary for lazy shader loading.
    explicit PostProcessor(const std::shared_ptr<ShaderLibrary>& shaderLibrary);

    /// @brief Destroys all GPU resources (framebuffers, textures).
    ~PostProcessor();

    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;
    PostProcessor(PostProcessor&&) = delete;
    PostProcessor& operator=(PostProcessor&&) = delete;

    /// @brief Prepares GPU resources required for overlay-only rendering.
    void prepareOverlayResources();

    /// @brief Prepares GPU resources required for full world rendering.
    void prepareWorldResources();

    /// @brief Resizes all internal framebuffers and textures to the new resolution.
    /// @param width New framebuffer width in pixels.
    /// @param height New framebuffer height in pixels.
    void resize(int width, int height);

    /// @brief Begins a new scene render pass; binds the scene color/depth framebuffer.
    void beginScene() const;

    /// @brief Composites lighting contributions and extracts bright-pass for bloom.
    /// @param atmosphereTextureId Texture ID containing atmospheric/volumetric lighting.
    /// @param bloomThreshold Luminance threshold above which pixels contribute to bloom.
    void composeLighting(unsigned int atmosphereTextureId, float bloomThreshold) const;

    /// @brief Completes the scene with post-processing effects and presents to backbuffer.
    /// @param settings Post-processing configuration (bloom strength, tonemap mode, etc.).
    /// @param debugView Debug view mode settings (wireframe, depth visualization, etc.).
    /// @param presentRuntimeOverlay Whether to composite the runtime overlay on top.
    void endScene(const PostProcessSettings& settings, const DebugViewSettings& debugView,
                  bool presentRuntimeOverlay) const;

    /// @brief Completes an overlay-only render pass.
    void endOverlayScene() const;

    /// @brief Copies the scene depth texture to the backbuffer (for debugging).
    void restoreSceneDepthToBackbuffer() const;

    /// @brief Draws all configured runtime overlay layers.
    void drawRuntimeOverlayLayers() const;

    /// @brief Sets the primary runtime overlay texture with default options.
    /// @param textureId OpenGL texture ID for the overlay.
    /// @param width Texture width in pixels.
    /// @param height Texture height in pixels.
    void setRuntimeOverlayTexture(unsigned int textureId, int width, int height) noexcept;

    /// @brief Sets the primary runtime overlay texture with custom layout options.
    /// @param textureId OpenGL texture ID for the overlay.
    /// @param width Texture width in pixels.
    /// @param height Texture height in pixels.
    /// @param options Layout, opacity, and effect configuration.
    void setRuntimeOverlayTexture(unsigned int textureId, int width, int height,
                                  const RuntimeOverlayOptions& options) noexcept;

    /// @brief Sets the secondary runtime overlay texture with default options.
    /// @param textureId OpenGL texture ID for the overlay.
    /// @param width Texture width in pixels.
    /// @param height Texture height in pixels.
    void setSecondaryRuntimeOverlayTexture(unsigned int textureId, int width, int height) noexcept;

    /// @brief Sets the secondary runtime overlay texture with custom layout options.
    /// @param textureId OpenGL texture ID for the overlay.
    /// @param width Texture width in pixels.
    /// @param height Texture height in pixels.
    /// @param options Layout, opacity, and effect configuration.
    void setSecondaryRuntimeOverlayTexture(unsigned int textureId, int width, int height,
                                           const RuntimeOverlayOptions& options) noexcept;

    /// @brief Presents the scene color texture directly to the backbuffer (no post-processing).
    void presentSceneTexture() const;

    /// @brief Presents a debug view of internal render targets.
    /// @param settings Post-processing settings controlling which debug view is active.
    /// @param debugView Debug view configuration.
    /// @param bloomTextureId Texture ID of the bloom pass output.
    void presentDebugView(const PostProcessSettings& settings, const DebugViewSettings& debugView,
                          unsigned int bloomTextureId) const;

    /// @brief Returns the OpenGL texture ID for the scene color buffer.
    /// @return Unsigned integer texture handle.
    unsigned int sceneTextureId() const noexcept;

    /// @brief Returns the OpenGL texture ID for the scene depth buffer.
    /// @return Unsigned integer texture handle.
    unsigned int sceneDepthTextureId() const noexcept;

  private:
    void createBuffers(int width, int height);
    void destroyBuffers() noexcept;
    void ensureOverlayShader() const;
    void ensureWorldShaders() const;
    void drawRuntimeOverlay() const;
    void drawRuntimeOverlayLayer(unsigned int textureId, int width, int height,
                                 const RuntimeOverlayOptions& options) const;

    int m_width = 1;
    int m_height = 1;
    unsigned int m_sceneFramebufferId = 0;
    unsigned int m_sceneColorTextureId = 0;
    unsigned int m_sceneDepthTextureId = 0;
    unsigned int m_compositeFramebufferId = 0;
    unsigned int m_compositeColorTextureId = 0;
    unsigned int m_compositeBrightTextureId = 0;
    unsigned int m_pingPongFramebufferIds[2] = {0, 0};
    unsigned int m_pingPongTextureIds[2] = {0, 0};
    FullScreenPass m_fullScreenPass;
    std::shared_ptr<ShaderLibrary> m_shaderLibrary;
    mutable const Shader* m_blurShader = nullptr;
    mutable const Shader* m_compositionShader = nullptr;
    mutable const Shader* m_tonemapShader = nullptr;
    mutable const Shader* m_overlayShader = nullptr;
    unsigned int m_runtimeOverlayTextureId = 0;
    int m_runtimeOverlayTextureWidth = 0;
    int m_runtimeOverlayTextureHeight = 0;
    RuntimeOverlayOptions m_runtimeOverlayOptions{};
    unsigned int m_secondaryRuntimeOverlayTextureId = 0;
    int m_secondaryRuntimeOverlayTextureWidth = 0;
    int m_secondaryRuntimeOverlayTextureHeight = 0;
    RuntimeOverlayOptions m_secondaryRuntimeOverlayOptions{};
};
} // namespace engine
