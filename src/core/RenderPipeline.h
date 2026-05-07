#pragma once

#include "core/Renderer.h"
#include "systems/RenderSystem.h"

namespace engine
{
class RenderPipeline final
{
  public:
    explicit RenderPipeline(Renderer& renderer);

    void renderFrame(const systems::RenderSceneView& renderSceneView, const Color& clearColor,
                     const PostProcessSettings& postProcessSettings,
                     const FrameUniforms& frameUniforms, float timeSeconds) const;

  private:
    Renderer& m_renderer;
};
} // namespace engine
