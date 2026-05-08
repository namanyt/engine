#include "core/RayEvaluationPass.h"

#include "core/RenderDebug.h"
#include "core/Renderer.h"
#include "core/Shader.h"

#include <glad/glad.h>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace
{
void allocateColorTexture(unsigned int textureId, int width, int height,
                          unsigned int internalFormat, unsigned int format, const char* label)
{
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), width, height, 0, format,
                 GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    engine::labelGlObject(GL_TEXTURE, textureId, label);
}

void validateFramebuffer(unsigned int framebufferId, const char* label)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error(std::string(label) + " framebuffer is incomplete.");
    }
}
} // namespace

namespace engine
{
RayEvaluationPass::RayEvaluationPass(const std::shared_ptr<ShaderLibrary>& shaderLibrary)
    : m_fullScreenPass(), m_shaderLibrary(shaderLibrary)
{
    createResources(m_fullWidth, m_fullHeight);
}

RayEvaluationPass::~RayEvaluationPass()
{
    destroyResources();
}

void RayEvaluationPass::prepareWorldResources()
{
    ensureShaders();
}

void RayEvaluationPass::resize(int width, int height)
{
    const int safeWidth = width > 0 ? width : 1;
    const int safeHeight = height > 0 ? height : 1;

    if (safeWidth == m_fullWidth && safeHeight == m_fullHeight)
    {
        return;
    }

    destroyResources();
    createResources(safeWidth, safeHeight);
}

void RayEvaluationPass::evaluate(unsigned int sceneDepthTextureId,
                                 const FrameUniforms& frameUniforms) const
{
    ensureShaders();
    const unsigned int writeIndex = m_historyWriteIndex;
    const unsigned int readIndex = (m_historyWriteIndex + 1u) % 2u;

    {
        ScopedRenderDebugGroup volumetricScope("Volumetric Fog Pass");
        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, m_halfFramebufferIds[writeIndex]);
        glViewport(0, 0, m_halfWidth, m_halfHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        m_evaluateShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneDepthTextureId);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_historyTextureIds[readIndex]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_halfDepthTextureIds[readIndex]);
        m_evaluateShader->setInt("uSceneDepthTexture", 0);
        m_evaluateShader->setInt("uPreviousHistoryTexture", 1);
        m_evaluateShader->setInt("uPreviousDepthTexture", 2);
        m_evaluateShader->setFloat("uShadowStrength", frameUniforms.rayEvaluation.shadowStrength);
        m_evaluateShader->setFloat("uScatteringStrength",
                                   frameUniforms.rayEvaluation.scatteringStrength);
        m_evaluateShader->setFloat("uVolumetricLightIntensity",
                                   frameUniforms.rayEvaluation.volumetricLightIntensity);
        m_evaluateShader->setFloat("uDirectionalLightAngularRadius",
                                   frameUniforms.rayEvaluation.directionalLightAngularRadius);
        m_evaluateShader->setFloat("uStepLength", frameUniforms.rayEvaluation.stepLength);
        m_evaluateShader->setFloat("uMaxDistance", frameUniforms.rayEvaluation.maxDistance);
        m_evaluateShader->setInt("uMaxSteps", frameUniforms.rayEvaluation.maxSteps);
        m_evaluateShader->setFloat("uExtinctionStrength",
                                   frameUniforms.rayEvaluation.extinctionStrength);
        m_evaluateShader->setFloat("uAtmosphericAmbientFloor",
                                   frameUniforms.rayEvaluation.atmosphericAmbientFloor);
        m_evaluateShader->setFloat("uTemporalBlend", frameUniforms.rayEvaluation.temporalBlend);
        m_evaluateShader->setFloat("uTemporalDepthThreshold",
                                   frameUniforms.rayEvaluation.temporalDepthThreshold);
        m_evaluateShader->setFloat("uTemporalNormalThreshold",
                                   frameUniforms.rayEvaluation.temporalNormalThreshold);
        m_evaluateShader->setFloat("uTemporalVelocityThreshold",
                                   frameUniforms.rayEvaluation.temporalVelocityThreshold);
        m_evaluateShader->setFloat("uNearFieldHaze", frameUniforms.rayEvaluation.nearFieldHaze);
        m_evaluateShader->setFloat("uPhaseAnisotropy", frameUniforms.rayEvaluation.phaseAnisotropy);
        m_evaluateShader->setFloat("uJitterStrength", frameUniforms.rayEvaluation.jitterStrength);
        m_evaluateShader->setFloat("uStepDistributionExponent",
                                   frameUniforms.rayEvaluation.stepDistributionExponent);
        m_evaluateShader->setFloat(
            "uTemporalJitterPhase",
            static_cast<float>(frameUniforms.frameIndex) *
                std::max(frameUniforms.rayEvaluation.temporalJitterScale, 0.0f));
        m_evaluateShader->setInt("uTemporalFrameIndex", frameUniforms.frameIndex);
        m_evaluateShader->setVec2("uFullResolution", static_cast<float>(m_fullWidth),
                                  static_cast<float>(m_fullHeight));
        m_evaluateShader->setVec2("uHalfResolution", static_cast<float>(m_halfWidth),
                                  static_cast<float>(m_halfHeight));
        m_evaluateShader->setVec3("uFogColor", frameUniforms.fogColor.x, frameUniforms.fogColor.y,
                                  frameUniforms.fogColor.z);
        m_evaluateShader->setFloat("uFogDensity", frameUniforms.fogDensity);
        m_evaluateShader->setFloat("uFogBaseHeight", frameUniforms.fogBaseHeight);
        m_evaluateShader->setFloat("uFogHeightFalloff", frameUniforms.fogHeightFalloff);
        m_evaluateShader->setFloat("uFogMaxHeight", frameUniforms.fogMaxHeight);
        m_evaluateShader->setMat4("uInverseProjection", frameUniforms.inverseProjectionMatrix);
        m_evaluateShader->setMat4("uPreviousInverseProjection",
                                  frameUniforms.previousInverseProjectionMatrix);
        m_evaluateShader->setMat4("uInverseView", frameUniforms.inverseViewMatrix);
        m_evaluateShader->setMat4("uPreviousViewProjection",
                                  frameUniforms.previousViewProjectionMatrix);
        m_evaluateShader->setVec3("uViewPosition", frameUniforms.viewPosition.x,
                                  frameUniforms.viewPosition.y, frameUniforms.viewPosition.z);
        m_evaluateShader->setVec3("uPreviousViewPosition", frameUniforms.previousViewPosition.x,
                                  frameUniforms.previousViewPosition.y,
                                  frameUniforms.previousViewPosition.z);
        m_evaluateShader->setVec3("uViewForward", frameUniforms.viewForward.x,
                                  frameUniforms.viewForward.y, frameUniforms.viewForward.z);
        m_evaluateShader->setVec3("uPreviousViewForward", frameUniforms.previousViewForward.x,
                                  frameUniforms.previousViewForward.y,
                                  frameUniforms.previousViewForward.z);
        m_evaluateShader->setVec3("uSunDirection", frameUniforms.directionalLight.direction.x,
                                  frameUniforms.directionalLight.direction.y,
                                  frameUniforms.directionalLight.direction.z);
        m_evaluateShader->setVec3("uPreviousLightDirection", frameUniforms.previousLightDirection.x,
                                  frameUniforms.previousLightDirection.y,
                                  frameUniforms.previousLightDirection.z);
        m_evaluateShader->setVec3("uSunColor", frameUniforms.directionalLight.color.x,
                                  frameUniforms.directionalLight.color.y,
                                  frameUniforms.directionalLight.color.z);
        m_evaluateShader->setFloat("uSunIntensity", frameUniforms.directionalLight.intensity);
        m_evaluateShader->setVec3("uSkyHorizonColor", frameUniforms.skyLight.horizonColor.x,
                                  frameUniforms.skyLight.horizonColor.y,
                                  frameUniforms.skyLight.horizonColor.z);
        m_evaluateShader->setVec3("uSkyZenithColor", frameUniforms.skyLight.zenithColor.x,
                                  frameUniforms.skyLight.zenithColor.y,
                                  frameUniforms.skyLight.zenithColor.z);
        m_evaluateShader->setVec3("uGroundAmbientColor", frameUniforms.skyLight.groundColor.x,
                                  frameUniforms.skyLight.groundColor.y,
                                  frameUniforms.skyLight.groundColor.z);
        m_evaluateShader->setFloat("uSkyIntensity", frameUniforms.skyLight.intensity);
        m_evaluateShader->setFloat("uSkyLightingEnabled",
                                   frameUniforms.debugView.skyLightingEnabled ? 1.0f : 0.0f);
        m_evaluateShader->setFloat("uFogEnabled", frameUniforms.debugView.fogEnabled ? 1.0f : 0.0f);
        m_evaluateShader->setFloat("uHistoryValid", m_historyValid ? 1.0f : 0.0f);
        constexpr std::size_t kMaxLocalLights = 8;
        constexpr std::size_t kMaxRayOccluders = 12;
        const std::size_t lightCount = std::min(frameUniforms.localLights.size(), kMaxLocalLights);
        const std::size_t occluderCount =
            std::min(frameUniforms.rayTracingScene.bounds.size(), kMaxRayOccluders);
        m_evaluateShader->setInt("uLocalLightCount", static_cast<int>(lightCount));
        m_evaluateShader->setInt("uRayOccluderCount", static_cast<int>(occluderCount));

        for (std::size_t index = 0; index < lightCount; ++index)
        {
            const LocalLight& light = frameUniforms.localLights[index];
            const std::string prefix = "uLocalLights[" + std::to_string(index) + "].";
            m_evaluateShader->setVec3(prefix + "position", light.position.x, light.position.y,
                                      light.position.z);
            m_evaluateShader->setVec3(prefix + "color", light.color.x, light.color.y,
                                      light.color.z);
            m_evaluateShader->setFloat(prefix + "intensity", light.intensity);
            m_evaluateShader->setFloat(prefix + "range", light.range);
        }

        for (std::size_t index = 0; index < occluderCount; ++index)
        {
            const BoundingSphere& bound = frameUniforms.rayTracingScene.bounds[index];
            const std::string prefix = "uRayOccluders[" + std::to_string(index) + "].";
            m_evaluateShader->setVec3(prefix + "center", bound.center.x, bound.center.y,
                                      bound.center.z);
            m_evaluateShader->setFloat(prefix + "radius", bound.radius);
        }

        m_fullScreenPass.draw();
    }

    {
        ScopedRenderDebugGroup atmosphereScope("Atmosphere Pass");
        glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFramebufferId);
        glViewport(0, 0, m_fullWidth, m_fullHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        m_resolveShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_historyTextureIds[writeIndex]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_halfMetricsTextureId);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_halfAuxTextureId);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, m_halfDepthTextureIds[writeIndex]);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, m_halfTemporalTextureIds[writeIndex]);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, sceneDepthTextureId);
        m_resolveShader->setInt("uHalfAtmosphereTexture", 0);
        m_resolveShader->setInt("uHalfMetricsTexture", 1);
        m_resolveShader->setInt("uHalfAuxTexture", 2);
        m_resolveShader->setInt("uHalfDepthTexture", 3);
        m_resolveShader->setInt("uHalfTemporalTexture", 4);
        m_resolveShader->setInt("uSceneDepthTexture", 5);
        m_resolveShader->setMat4("uInverseProjection", frameUniforms.inverseProjectionMatrix);
        m_resolveShader->setFloat("uBilateralDepthFactor",
                                  frameUniforms.rayEvaluation.bilateralDepthFactor);
        m_resolveShader->setVec2("uFullResolution", static_cast<float>(m_fullWidth),
                                 static_cast<float>(m_fullHeight));
        m_resolveShader->setVec2("uHalfResolution", static_cast<float>(m_halfWidth),
                                 static_cast<float>(m_halfHeight));
        m_resolveShader->setInt("uVolumetricDebugViewMode",
                                frameUniforms.debugView.volumetricDebugViewMode);
        m_fullScreenPass.draw();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
    m_historyWriteIndex = (m_historyWriteIndex + 1u) % 2u;
    m_historyValid = true;
}

unsigned int RayEvaluationPass::atmosphereTextureId() const noexcept
{
    return m_atmosphereTextureId;
}

int RayEvaluationPass::renderWidth() const noexcept
{
    return m_halfWidth;
}

int RayEvaluationPass::renderHeight() const noexcept
{
    return m_halfHeight;
}

void RayEvaluationPass::ensureShaders() const
{
    if (m_evaluateShader == nullptr)
    {
        m_evaluateShader = &m_shaderLibrary->loadGraphicsProgram(
            "renderer.volumetric.evaluate", std::filesystem::path("ray_eval.vert"),
            std::filesystem::path("ray_eval.frag"));
    }

    if (m_resolveShader == nullptr)
    {
        m_resolveShader = &m_shaderLibrary->loadGraphicsProgram(
            "renderer.volumetric.resolve", std::filesystem::path("ray_eval.vert"),
            std::filesystem::path("volumetric_upscale.frag"));
    }
}

void RayEvaluationPass::createResources(int width, int height)
{
    m_fullWidth = width;
    m_fullHeight = height;
    m_halfWidth = std::max(1, width / 2);
    m_halfHeight = std::max(1, height / 2);

    glGenTextures(2, m_historyTextureIds);
    for (int index = 0; index < 2; ++index)
    {
        allocateColorTexture(m_historyTextureIds[index], m_halfWidth, m_halfHeight, GL_RGBA16F,
                             GL_RGBA, index == 0 ? "Volumetric.HistoryA" : "Volumetric.HistoryB");
    }

    glGenTextures(2, m_halfDepthTextureIds);
    for (int index = 0; index < 2; ++index)
    {
        allocateColorTexture(m_halfDepthTextureIds[index], m_halfWidth, m_halfHeight, GL_R16F,
                             GL_RED,
                             index == 0 ? "Volumetric.HalfDepthA" : "Volumetric.HalfDepthB");
    }

    glGenTextures(2, m_halfTemporalTextureIds);
    for (int index = 0; index < 2; ++index)
    {
        allocateColorTexture(m_halfTemporalTextureIds[index], m_halfWidth, m_halfHeight, GL_RGBA16F,
                             GL_RGBA, index == 0 ? "Volumetric.TemporalA" : "Volumetric.TemporalB");
    }

    glGenTextures(1, &m_halfMetricsTextureId);
    allocateColorTexture(m_halfMetricsTextureId, m_halfWidth, m_halfHeight, GL_RGBA16F, GL_RGBA,
                         "Volumetric.Metrics");

    glGenTextures(1, &m_halfAuxTextureId);
    allocateColorTexture(m_halfAuxTextureId, m_halfWidth, m_halfHeight, GL_RGBA16F, GL_RGBA,
                         "Volumetric.AuxMetrics");

    glGenFramebuffers(2, m_halfFramebufferIds);
    for (int index = 0; index < 2; ++index)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_halfFramebufferIds[index]);
        labelGlObject(GL_FRAMEBUFFER, m_halfFramebufferIds[index],
                      index == 0 ? "Volumetric.EvaluateFramebufferA"
                                 : "Volumetric.EvaluateFramebufferB");
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               m_historyTextureIds[index], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                               m_halfMetricsTextureId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                               m_halfAuxTextureId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D,
                               m_halfDepthTextureIds[index], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D,
                               m_halfTemporalTextureIds[index], 0);
        const unsigned int attachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                            GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
                                            GL_COLOR_ATTACHMENT4};
        glDrawBuffers(5, attachments);
        validateFramebuffer(m_halfFramebufferIds[index], "Volumetric evaluate");
    }

    glGenFramebuffers(1, &m_resolveFramebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_resolveFramebufferId);
    labelGlObject(GL_FRAMEBUFFER, m_resolveFramebufferId, "Volumetric.ResolveFramebuffer");

    glGenTextures(1, &m_atmosphereTextureId);
    allocateColorTexture(m_atmosphereTextureId, m_fullWidth, m_fullHeight, GL_RGBA16F, GL_RGBA,
                         "Volumetric.AtmosphereTexture");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_atmosphereTextureId, 0);
    const unsigned int resolveAttachment = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &resolveAttachment);
    validateFramebuffer(m_resolveFramebufferId, "Volumetric resolve");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    invalidateHistory();
}

void RayEvaluationPass::destroyResources() noexcept
{
    glDeleteTextures(2, m_historyTextureIds);
    glDeleteTextures(2, m_halfDepthTextureIds);
    glDeleteTextures(2, m_halfTemporalTextureIds);
    glDeleteTextures(1, &m_halfMetricsTextureId);
    glDeleteTextures(1, &m_halfAuxTextureId);
    glDeleteFramebuffers(2, m_halfFramebufferIds);
    glDeleteTextures(1, &m_atmosphereTextureId);
    glDeleteFramebuffers(1, &m_resolveFramebufferId);

    m_historyTextureIds[0] = 0;
    m_historyTextureIds[1] = 0;
    m_halfDepthTextureIds[0] = 0;
    m_halfDepthTextureIds[1] = 0;
    m_halfTemporalTextureIds[0] = 0;
    m_halfTemporalTextureIds[1] = 0;
    m_halfMetricsTextureId = 0;
    m_halfAuxTextureId = 0;
    m_halfFramebufferIds[0] = 0;
    m_halfFramebufferIds[1] = 0;
    m_atmosphereTextureId = 0;
    m_resolveFramebufferId = 0;
    invalidateHistory();
}

void RayEvaluationPass::invalidateHistory() noexcept
{
    m_historyWriteIndex = 0;
    m_historyValid = false;
}
} // namespace engine
