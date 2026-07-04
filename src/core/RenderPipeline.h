#pragma once

#include "core/Renderer.h"
#include "systems/RenderSystem.h"

namespace engine
{
/**
 * @brief Orchestrates full-frame rendering passes through the Renderer.
 *
 * `RenderPipeline` is a thin coordinator that delegates to the `Renderer`
 * for world and overlay frame rendering. It encapsulates the sequence of
 * shadow, geometry, post-processing, and presentation steps.
 *
 * @see Renderer
 * @see PostProcessor
 */
class RenderPipeline final
{
  public:
    /// @brief Constructs a render pipeline bound to the given renderer.
    /// @param renderer Reference to the active Renderer instance.
    explicit RenderPipeline(Renderer& renderer);

    /// @brief Renders a complete world frame with post-processing.
    /// @param renderSceneView ECS scene view for entities to render.
    /// @param clearColor Clear color for the framebuffer.
    /// @param postProcessSettings Post-processing configuration (bloom, tonemap, etc.).
    /// @param frameUniforms Per-frame uniform data (matrices, lights, fog, etc.).
    /// @param timeSeconds Current application time in seconds.
    void renderFrame(const systems::RenderSceneView& renderSceneView, const Color& clearColor,
                     const PostProcessSettings& postProcessSettings,
                     const FrameUniforms& frameUniforms, float timeSeconds) const;

    /// @brief Renders an overlay-only frame (UI, HUD, debug overlays).
    /// @param clearColor Clear color for the overlay framebuffer.
    void renderOverlayFrame(const Color& clearColor) const;

  private:
    Renderer& m_renderer;
};
} // namespace engine
