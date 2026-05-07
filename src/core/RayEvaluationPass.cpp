#include "core/RayEvaluationPass.h"

#include "core/Renderer.h"
#include "core/Shader.h"

#include <glad/glad.h>

#include <string>

namespace engine
{
RayEvaluationPass::RayEvaluationPass(const std::shared_ptr<ShaderLibrary>& shaderLibrary)
    : m_fullScreenPass(), m_shaderLibrary(shaderLibrary)
{
    m_computeShader = std::make_unique<Shader>(m_shaderLibrary->shaderPath("ray_eval.vert"),
                                               m_shaderLibrary->shaderPath("ray_eval.frag"));
    createResources(m_width, m_height);
}

RayEvaluationPass::~RayEvaluationPass()
{
    destroyResources();
}

void RayEvaluationPass::resize(int width, int height)
{
    const int safeWidth = width > 0 ? width : 1;
    const int safeHeight = height > 0 ? height : 1;

    if (safeWidth == m_width && safeHeight == m_height)
    {
        return;
    }

    destroyResources();
    createResources(safeWidth, safeHeight);
}

void RayEvaluationPass::evaluate(unsigned int sceneDepthTextureId,
                                 unsigned int sceneLightingTextureId,
                                 const FrameUniforms& frameUniforms) const
{
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    m_computeShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneDepthTextureId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sceneLightingTextureId);
    m_computeShader->setInt("uSceneDepthTexture", 0);
    m_computeShader->setInt("uSurfaceLightingTexture", 1);
    m_computeShader->setFloat("uShadowStrength", frameUniforms.rayEvaluation.shadowStrength);
    m_computeShader->setFloat("uAtmosphereIntensity",
                              frameUniforms.rayEvaluation.atmosphereIntensity);
    m_computeShader->setFloat("uEmissiveScatter", frameUniforms.rayEvaluation.emissiveScatter);
    m_computeShader->setFloat("uStepLength", frameUniforms.rayEvaluation.stepLength);
    m_computeShader->setFloat("uMaxDistance", frameUniforms.rayEvaluation.maxDistance);
    m_computeShader->setInt("uMaxSteps", frameUniforms.rayEvaluation.maxSteps);
    m_computeShader->setFloat("uExtinction", frameUniforms.rayEvaluation.extinction);
    m_computeShader->setFloat("uNearFieldHaze", frameUniforms.rayEvaluation.nearFieldHaze);
    m_computeShader->setFloat("uPhaseAnisotropy", frameUniforms.rayEvaluation.phaseAnisotropy);
    m_computeShader->setFloat("uJitterStrength", frameUniforms.rayEvaluation.jitterStrength);
    m_computeShader->setFloat("uStepDistributionExponent",
                              frameUniforms.rayEvaluation.stepDistributionExponent);
    m_computeShader->setFloat("uTemporalJitterPhase",
                              frameUniforms.rayEvaluation.temporalJitterScale > 0.0f
                                  ? frameUniforms.timeSeconds *
                                        frameUniforms.rayEvaluation.temporalJitterScale
                                  : 0.0f);
    m_computeShader->setVec3("uFogColor", frameUniforms.fogColor.x, frameUniforms.fogColor.y,
                             frameUniforms.fogColor.z);
    m_computeShader->setFloat("uFogDensity", frameUniforms.fogDensity);
    m_computeShader->setFloat("uFogBaseHeight", frameUniforms.fogBaseHeight);
    m_computeShader->setFloat("uFogHeightFalloff", frameUniforms.fogHeightFalloff);
    m_computeShader->setVec3("uViewPosition", frameUniforms.viewPosition.x,
                             frameUniforms.viewPosition.y, frameUniforms.viewPosition.z);
    m_computeShader->setVec3("uViewForward", frameUniforms.viewForward.x,
                             frameUniforms.viewForward.y, frameUniforms.viewForward.z);
    m_computeShader->setVec3("uViewRight", frameUniforms.viewRight.x, frameUniforms.viewRight.y,
                             frameUniforms.viewRight.z);
    m_computeShader->setVec3("uViewUp", frameUniforms.viewUp.x, frameUniforms.viewUp.y,
                             frameUniforms.viewUp.z);
    m_computeShader->setFloat("uAspectRatio", frameUniforms.aspectRatio);
    m_computeShader->setFloat("uVerticalFieldOfViewRadians",
                              frameUniforms.verticalFieldOfViewRadians);
    m_computeShader->setFloat("uNearPlane", frameUniforms.nearPlane);
    m_computeShader->setVec3("uSunDirection", frameUniforms.directionalLight.direction.x,
                             frameUniforms.directionalLight.direction.y,
                             frameUniforms.directionalLight.direction.z);
    m_computeShader->setVec3("uSunColor", frameUniforms.directionalLight.color.x,
                             frameUniforms.directionalLight.color.y,
                             frameUniforms.directionalLight.color.z);
    m_computeShader->setFloat("uSunIntensity", frameUniforms.directionalLight.intensity);
    m_computeShader->setVec3("uSkyHorizonColor", frameUniforms.skyLight.horizonColor.x,
                             frameUniforms.skyLight.horizonColor.y,
                             frameUniforms.skyLight.horizonColor.z);
    m_computeShader->setVec3("uSkyZenithColor", frameUniforms.skyLight.zenithColor.x,
                             frameUniforms.skyLight.zenithColor.y,
                             frameUniforms.skyLight.zenithColor.z);
    m_computeShader->setVec3("uGroundAmbientColor", frameUniforms.skyLight.groundColor.x,
                             frameUniforms.skyLight.groundColor.y,
                             frameUniforms.skyLight.groundColor.z);
    m_computeShader->setFloat("uSkyIntensity", frameUniforms.skyLight.intensity);
    m_computeShader->setFloat("uSkyLightingEnabled",
                              frameUniforms.debugView.skyLightingEnabled ? 1.0f : 0.0f);
    m_computeShader->setFloat("uFogEnabled", frameUniforms.debugView.fogEnabled ? 1.0f : 0.0f);
    m_computeShader->setInt("uVolumetricDebugViewMode",
                            frameUniforms.debugView.volumetricDebugViewMode);
    constexpr std::size_t kMaxLocalLights = 8;
    constexpr std::size_t kMaxRayOccluders = 12;
    const std::size_t lightCount = std::min(frameUniforms.localLights.size(), kMaxLocalLights);
    const std::size_t occluderCount =
        std::min(frameUniforms.rayTracingScene.bounds.size(), kMaxRayOccluders);
    m_computeShader->setInt("uLocalLightCount", static_cast<int>(lightCount));
    m_computeShader->setInt("uRayOccluderCount", static_cast<int>(occluderCount));

    for (std::size_t index = 0; index < lightCount; ++index)
    {
        const LocalLight& light = frameUniforms.localLights[index];
        const std::string prefix = "uLocalLights[" + std::to_string(index) + "].";
        m_computeShader->setVec3(prefix + "position", light.position.x, light.position.y,
                                 light.position.z);
        m_computeShader->setVec3(prefix + "color", light.color.x, light.color.y, light.color.z);
        m_computeShader->setFloat(prefix + "intensity", light.intensity);
        m_computeShader->setFloat(prefix + "range", light.range);
    }

    for (std::size_t index = 0; index < occluderCount; ++index)
    {
        const BoundingSphere& bound = frameUniforms.rayTracingScene.bounds[index];
        const std::string prefix = "uRayOccluders[" + std::to_string(index) + "].";
        m_computeShader->setVec3(prefix + "center", bound.center.x, bound.center.y, bound.center.z);
        m_computeShader->setFloat(prefix + "radius", bound.radius);
    }

    m_fullScreenPass.draw();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
}

unsigned int RayEvaluationPass::atmosphereTextureId() const noexcept
{
    return m_atmosphereTextureId;
}

void RayEvaluationPass::createResources(int width, int height)
{
    m_width = width;
    m_height = height;

    glGenFramebuffers(1, &m_framebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);

    glGenTextures(1, &m_atmosphereTextureId);
    glBindTexture(GL_TEXTURE_2D, m_atmosphereTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_atmosphereTextureId, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Atmosphere framebuffer is incomplete.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RayEvaluationPass::destroyResources() noexcept
{
    glDeleteTextures(1, &m_atmosphereTextureId);
    glDeleteFramebuffers(1, &m_framebufferId);
    m_atmosphereTextureId = 0;
    m_framebufferId = 0;
}
} // namespace engine
