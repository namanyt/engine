#include "core/RenderPipeline.h"

#include "core/RenderDebug.h"

#include <glad/glad.h>

#include <algorithm>

namespace
{
void drawRenderItem(engine::Renderer& renderer, const engine::systems::RenderItem& item,
                    const engine::FrameUniforms& frameUniforms)
{
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean cullFaceWasEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean polygonOffsetLineWasEnabled = glIsEnabled(GL_POLYGON_OFFSET_LINE);
    GLboolean depthWriteWasEnabled = GL_TRUE;
    GLfloat previousLineWidth = 1.0f;
    GLint polygonMode[2]{GL_FILL, GL_FILL};

    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteWasEnabled);
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);
    glGetIntegerv(GL_POLYGON_MODE, polygonMode);

    if (item.depthTest)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask(item.depthWrite ? GL_TRUE : GL_FALSE);

    if (item.doubleSided)
    {
        glDisable(GL_CULL_FACE);
    }
    else if (cullFaceWasEnabled)
    {
        glEnable(GL_CULL_FACE);
    }

    if (item.alphaBlended)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    if (item.wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(std::max(item.lineWidth, 1.0f));
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f, -1.0f);
    }

    renderer.draw(*item.mesh, item.material, item.modelMatrix, frameUniforms, item.textureId,
                  item.opacity);

    glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
    glLineWidth(previousLineWidth);
    if (polygonOffsetLineWasEnabled)
    {
        glEnable(GL_POLYGON_OFFSET_LINE);
    }
    else
    {
        glDisable(GL_POLYGON_OFFSET_LINE);
    }

    if (depthTestWasEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    if (blendWasEnabled)
    {
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    if (cullFaceWasEnabled)
    {
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }

    glDepthMask(depthWriteWasEnabled);
}
} // namespace

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

            drawRenderItem(m_renderer, item, frameUniforms);
        }
    }

    {
        ScopedRenderDebugGroup geometryGroup("Geometry Pass");
        const auto geometryGpuScope = m_renderer.profiler().makeGpuScope("Geometry Pass");
        for (const systems::RenderItem& item : renderSceneView.geometryItems)
        {
            if (item.mesh == nullptr || item.material.shader == nullptr || !item.visible ||
                item.alphaBlended || item.worldUi)
            {
                continue;
            }

            drawRenderItem(m_renderer, item, frameUniforms);
        }
    }

    {
        ScopedRenderDebugGroup transparentGroup("Transparent Geometry Pass");
        const auto transparentGpuScope =
            m_renderer.profiler().makeGpuScope("Transparent Geometry Pass");
        for (const systems::RenderItem& item : renderSceneView.geometryItems)
        {
            if (item.mesh == nullptr || item.material.shader == nullptr || !item.visible ||
                !item.alphaBlended || item.worldUi)
            {
                continue;
            }

            drawRenderItem(m_renderer, item, frameUniforms);
        }
    }

    m_renderer.endFrame(postProcessSettings, frameUniforms, timeSeconds, false);

    bool hasWorldUiItems = false;
    for (const systems::RenderItem& item : renderSceneView.geometryItems)
    {
        if (item.mesh != nullptr && item.material.shader != nullptr && item.visible && item.worldUi)
        {
            hasWorldUiItems = true;
            break;
        }
    }

    if (hasWorldUiItems)
    {
        m_renderer.restoreSceneDepthToBackbuffer();

        ScopedRenderDebugGroup worldUiGroup("World UI Pass");
        const auto worldUiGpuScope = m_renderer.profiler().makeGpuScope("World UI Pass");
        for (const systems::RenderItem& item : renderSceneView.geometryItems)
        {
            if (item.mesh == nullptr || item.material.shader == nullptr || !item.visible ||
                !item.worldUi)
            {
                continue;
            }

            drawRenderItem(m_renderer, item, frameUniforms);
        }
    }

    m_renderer.drawRuntimeOverlayLayers();
}

void RenderPipeline::renderOverlayFrame(const Color& clearColor) const
{
    m_renderer.beginFrame(clearColor);
    m_renderer.endOverlayFrame();
}
} // namespace engine
