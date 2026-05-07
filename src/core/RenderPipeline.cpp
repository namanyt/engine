#include "core/RenderPipeline.h"

#include "core/RenderDebug.h"

namespace engine
{
RenderPipeline::RenderPipeline(Renderer& renderer) : m_renderer(renderer) {}

void RenderPipeline::renderFrame(const systems::RenderSceneView& renderSceneView,
                                 const Color& clearColor,
                                 const PostProcessSettings& postProcessSettings,
                                 const FrameUniforms& frameUniforms, float timeSeconds) const
{
    const auto frameGpuScope = m_renderer.profiler().makeGpuScope("Frame");

    {
        const auto shadowCpuScope = m_renderer.profiler().makeCpuScope("Shadow Rendering");
        const auto shadowGpuScope = m_renderer.profiler().makeGpuScope("Shadow Pass");
        m_renderer.beginShadowPass(frameUniforms);

        for (const systems::RenderItem& item : renderSceneView.shadowCasters)
        {
            if (item.mesh == nullptr)
            {
                continue;
            }

            m_renderer.drawShadow(*item.mesh, item.modelMatrix);
        }

        m_renderer.endShadowPass();
    }

    m_renderer.beginFrame(clearColor);

    {
        ScopedRenderDebugGroup terrainGroup("Terrain Pass");
        const auto terrainGpuScope = m_renderer.profiler().makeGpuScope("Terrain Pass");
        for (const systems::RenderItem& item : renderSceneView.terrainItems)
        {
            if (item.mesh == nullptr || item.material.shader == nullptr || !item.visible)
            {
                continue;
            }

            m_renderer.draw(*item.mesh, item.material, item.modelMatrix, frameUniforms);
        }
    }

    {
        ScopedRenderDebugGroup geometryGroup("Geometry Pass");
        const auto geometryGpuScope = m_renderer.profiler().makeGpuScope("Geometry Pass");
        for (const systems::RenderItem& item : renderSceneView.geometryItems)
        {
            if (item.mesh == nullptr || item.material.shader == nullptr || !item.visible)
            {
                continue;
            }

            m_renderer.draw(*item.mesh, item.material, item.modelMatrix, frameUniforms);
        }
    }

    m_renderer.endFrame(postProcessSettings, frameUniforms, timeSeconds);
}
} // namespace engine
