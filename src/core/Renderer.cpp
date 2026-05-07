#include "core/Renderer.h"

#include "core/Log.h"
#include "core/PostProcessor.h"
#include "core/RayEvaluationPass.h"
#include "core/RenderDebug.h"
#include "core/Shader.h"
#include "core/ShaderLibrary.h"
#include "core/ShadowMapPass.h"
#include "graphics/Mesh.h"

#include <glad/glad.h>

#include <sstream>
#include <string>

namespace engine
{
Renderer::Renderer(const std::filesystem::path& shaderDirectory)
    : m_shaderLibrary(std::make_shared<ShaderLibrary>(shaderDirectory))
{
    glEnable(GL_DEPTH_TEST);
    m_postProcessor = std::make_unique<PostProcessor>(m_shaderLibrary);
    m_rayEvaluationPass = std::make_unique<RayEvaluationPass>(m_shaderLibrary);
    m_shadowMapPass = std::make_unique<ShadowMapPass>(m_shaderLibrary);
    if (m_postProcessor != nullptr)
    {
        m_postProcessor->resize(m_framebufferWidth, m_framebufferHeight);
    }

    Log::info("Renderer", "Renderer initialized.");
    Log::info("Renderer", "Depth testing enabled.");
}

Renderer::~Renderer() {}

void Renderer::setViewport(int width, int height)
{
    m_framebufferWidth = width > 0 ? width : 1;
    m_framebufferHeight = height > 0 ? height : 1;

    if (m_postProcessor != nullptr)
    {
        m_postProcessor->resize(m_framebufferWidth, m_framebufferHeight);
    }

    if (m_rayEvaluationPass != nullptr)
    {
        m_rayEvaluationPass->resize(m_framebufferWidth, m_framebufferHeight);
    }

    if (m_shadowMapPass != nullptr)
    {
        m_shadowMapPass->resize(2048);
    }

    glViewport(0, 0, m_framebufferWidth, m_framebufferHeight);
}

void Renderer::beginFrame(const Color& clearColor)
{
    if (m_postProcessor != nullptr)
    {
        m_postProcessor->beginScene();
    }

    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame(const PostProcessSettings& postProcessSettings,
                        const FrameUniforms& frameUniforms, float timeSeconds) const
{
    (void)timeSeconds;

    if (m_rayEvaluationPass != nullptr && m_postProcessor != nullptr)
    {
        const auto fogCpuScope = m_profiler.makeCpuScope("Fog Integration");
        const auto atmosphereGpuScope = m_profiler.makeGpuScope("Volumetric Fog Pass");
        m_rayEvaluationPass->evaluate(m_postProcessor->sceneDepthTextureId(), frameUniforms);
        m_profiler.setVolumetricStats(
            m_rayEvaluationPass->renderWidth(), m_rayEvaluationPass->renderHeight(),
            frameUniforms.rayEvaluation.maxSteps,
            static_cast<std::uint64_t>(m_rayEvaluationPass->renderWidth()) *
                static_cast<std::uint64_t>(m_rayEvaluationPass->renderHeight()) *
                static_cast<std::uint64_t>(std::max(frameUniforms.rayEvaluation.maxSteps, 0)));
    }

    if (m_postProcessor != nullptr && m_rayEvaluationPass != nullptr)
    {
        const auto lightingGpuScope = m_profiler.makeGpuScope("Lighting Pass");
        m_postProcessor->composeLighting(m_rayEvaluationPass->atmosphereTextureId(),
                                         postProcessSettings.bloomThreshold);
    }

    if (m_postProcessor != nullptr)
    {
        const auto postGpuScope = m_profiler.makeGpuScope("Post Processing Pass");
        m_postProcessor->endScene(postProcessSettings, frameUniforms.debugView);
    }
}

void Renderer::beginShadowPass(const FrameUniforms& frameUniforms) const
{
    if (m_shadowMapPass == nullptr)
    {
        return;
    }

    m_shadowMapPass->resize(frameUniforms.shadowSettings.mapSize);
    m_shadowMapPass->begin(frameUniforms.lightViewProjectionMatrix);
}

void Renderer::drawShadow(const Mesh& mesh, const Transform& transform) const
{
    drawShadow(mesh, transform.modelMatrix());
}

void Renderer::drawShadow(const Mesh& mesh, const Mat4& modelMatrix) const
{
    if (m_shadowMapPass == nullptr)
    {
        return;
    }

    m_profiler.addDrawCall();
    m_shadowMapPass->draw(mesh, modelMatrix);
}

void Renderer::endShadowPass() const
{
    if (m_shadowMapPass == nullptr)
    {
        return;
    }

    m_shadowMapPass->end(m_framebufferWidth, m_framebufferHeight);
}

void Renderer::draw(const Mesh& mesh, const Material& material, const Transform& transform,
                    const FrameUniforms& frameUniforms) const
{
    draw(mesh, material, transform.modelMatrix(), frameUniforms);
}

void Renderer::draw(const Mesh& mesh, const Material& material, const Mat4& modelMatrix,
                    const FrameUniforms& frameUniforms) const
{
    if (material.shader == nullptr)
    {
        return;
    }

    const Shader& shader = *material.shader;

    shader.use();
    shader.setMat4("uModel", modelMatrix);
    applyFrameState(shader, frameUniforms);
    applyMaterialState(shader, material);
    applyLocalLightState(shader, frameUniforms);

    if (m_shadowMapPass != nullptr)
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_shadowMapPass->depthTextureId());
        shader.setInt("uShadowMap", 2);
    }

    m_profiler.addDrawCall();
    mesh.draw();
}

ShaderLibrary& Renderer::shaderLibrary() noexcept
{
    return *m_shaderLibrary;
}

const ShaderLibrary& Renderer::shaderLibrary() const noexcept
{
    return *m_shaderLibrary;
}

RenderProfiler& Renderer::profiler() noexcept
{
    return m_profiler;
}

const RenderProfiler& Renderer::profiler() const noexcept
{
    return m_profiler;
}

RendererDebugTextures Renderer::debugTextures() const noexcept
{
    RendererDebugTextures textures{};

    if (m_shadowMapPass != nullptr)
    {
        textures.shadowMapTextureId = m_shadowMapPass->depthTextureId();
    }

    if (m_postProcessor != nullptr)
    {
        textures.sceneDepthTextureId = m_postProcessor->sceneDepthTextureId();
    }

    if (m_rayEvaluationPass != nullptr)
    {
        textures.volumetricTextureId = m_rayEvaluationPass->atmosphereTextureId();
    }

    return textures;
}

void Renderer::applyFrameState(const Shader& shader, const FrameUniforms& frameUniforms) const
{
    shader.setMat4("uView", frameUniforms.viewMatrix);
    shader.setMat4("uProjection", frameUniforms.projectionMatrix);
    shader.setMat4("uLightViewProjection", frameUniforms.lightViewProjectionMatrix);
    shader.setVec3("uViewPosition", frameUniforms.viewPosition.x, frameUniforms.viewPosition.y,
                   frameUniforms.viewPosition.z);
    shader.setVec3("uSunDirection", frameUniforms.directionalLight.direction.x,
                   frameUniforms.directionalLight.direction.y,
                   frameUniforms.directionalLight.direction.z);
    shader.setVec3("uSunColor", frameUniforms.directionalLight.color.x,
                   frameUniforms.directionalLight.color.y, frameUniforms.directionalLight.color.z);
    shader.setFloat("uSunIntensity", frameUniforms.directionalLight.intensity);
    shader.setVec3("uSkyHorizonColor", frameUniforms.skyLight.horizonColor.x,
                   frameUniforms.skyLight.horizonColor.y, frameUniforms.skyLight.horizonColor.z);
    shader.setVec3("uSkyZenithColor", frameUniforms.skyLight.zenithColor.x,
                   frameUniforms.skyLight.zenithColor.y, frameUniforms.skyLight.zenithColor.z);
    shader.setVec3("uGroundAmbientColor", frameUniforms.skyLight.groundColor.x,
                   frameUniforms.skyLight.groundColor.y, frameUniforms.skyLight.groundColor.z);
    shader.setFloat("uSkyIntensity", frameUniforms.skyLight.intensity);
    shader.setFloat("uAmbientEnabled", frameUniforms.debugView.ambientEnabled ? 1.0f : 0.0f);
    shader.setFloat("uSkyLightingEnabled",
                    frameUniforms.debugView.skyLightingEnabled ? 1.0f : 0.0f);
    shader.setFloat("uEmissiveEnabled",
                    frameUniforms.debugView.emissivePropagationEnabled ? 1.0f : 0.0f);
    shader.setInt("uMaterialDebugViewMode", frameUniforms.debugView.materialDebugViewMode);
    shader.setFloat("uTime", frameUniforms.timeSeconds);
    shader.setFloat("uShadowBias", frameUniforms.shadowSettings.bias);
    shader.setFloat("uShadowNormalBias", frameUniforms.shadowSettings.normalBias);
    shader.setFloat("uShadowStrength", frameUniforms.shadowSettings.strength);
}

void Renderer::applyMaterialState(const Shader& shader, const Material& material) const
{
    shader.setInt("uMaterialCategory", static_cast<int>(material.category));
    shader.setVec3("uMaterialAlbedo", material.albedo.x, material.albedo.y, material.albedo.z);
    shader.setVec3("uMaterialEmissiveColor", material.emissiveColor.x, material.emissiveColor.y,
                   material.emissiveColor.z);
    shader.setFloat("uMaterialEmissiveStrength", material.emissiveStrength);
    shader.setFloat("uMaterialRoughness", material.roughness);
    shader.setFloat("uMaterialMetallic", material.metallic);
    shader.setFloat("uMaterialSpecularStrength", material.specularStrength);
    shader.setFloat("uMaterialSoftness", material.softness);
    shader.setFloat("uMaterialAtmosphericResponse", material.atmosphericResponse);
}

void Renderer::applyLocalLightState(const Shader& shader, const FrameUniforms& frameUniforms) const
{
    constexpr std::size_t kMaxLocalLights = 8;
    const std::size_t lightCount = std::min(frameUniforms.localLights.size(), kMaxLocalLights);
    shader.setInt("uLocalLightCount", static_cast<int>(lightCount));

    for (std::size_t index = 0; index < lightCount; ++index)
    {
        const LocalLight& light = frameUniforms.localLights[index];
        const std::string prefix = "uLocalLights[" + std::to_string(index) + "].";
        shader.setVec3(prefix + "position", light.position.x, light.position.y, light.position.z);
        shader.setVec3(prefix + "color", light.color.x, light.color.y, light.color.z);
        shader.setFloat(prefix + "intensity", light.intensity);
        shader.setFloat(prefix + "range", light.range);
    }
}
} // namespace engine
