#include "core/PostProcessor.h"

#include "core/Shader.h"

#include <glad/glad.h>

#include <stdexcept>

namespace
{
float debugExposureScale(float exposure)
{
    return exposure > 0.0f ? exposure : 1.0f;
}

void attachColorTexture(unsigned int textureId, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void validateFramebuffer(unsigned int framebufferId, const char* label)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error(std::string("Framebuffer is incomplete: ") + label);
    }
}
} // namespace

namespace engine
{
PostProcessor::PostProcessor(const std::shared_ptr<ShaderLibrary>& shaderLibrary)
    : m_fullScreenPass(), m_shaderLibrary(shaderLibrary)
{
    m_blurShader = std::make_unique<Shader>(m_shaderLibrary->shaderPath("post_blur.vert"),
                                            m_shaderLibrary->shaderPath("post_blur.frag"));
    m_compositionShader =
        std::make_unique<Shader>(m_shaderLibrary->shaderPath("post_tonemap.vert"),
                                 m_shaderLibrary->shaderPath("post_compose.frag"));
    m_tonemapShader = std::make_unique<Shader>(m_shaderLibrary->shaderPath("post_tonemap.vert"),
                                               m_shaderLibrary->shaderPath("post_tonemap.frag"));
    createBuffers(m_width, m_height);
}

PostProcessor::~PostProcessor()
{
    destroyBuffers();
}

void PostProcessor::resize(int width, int height)
{
    const int safeWidth = width > 0 ? width : 1;
    const int safeHeight = height > 0 ? height : 1;

    if (safeWidth == m_width && safeHeight == m_height)
    {
        return;
    }

    destroyBuffers();
    createBuffers(safeWidth, safeHeight);
}

void PostProcessor::beginScene() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFramebufferId);
    glViewport(0, 0, m_width, m_height);
}

void PostProcessor::composeLighting(unsigned int atmosphereTextureId, float bloomThreshold) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_compositeFramebufferId);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);

    m_compositionShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneColorTextureId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_sceneDepthTextureId);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, atmosphereTextureId);
    m_compositionShader->setInt("uSceneTexture", 0);
    m_compositionShader->setInt("uSceneDepthTexture", 1);
    m_compositionShader->setInt("uAtmosphereTexture", 2);
    m_compositionShader->setFloat("uBloomThreshold", bloomThreshold);
    m_fullScreenPass.draw();
}

void PostProcessor::endScene(const PostProcessSettings& settings,
                             const DebugViewSettings& debugView) const
{
    unsigned int activeBloomTextureId = m_compositeBrightTextureId;

    if (debugView.postProcessingEnabled)
    {
        bool horizontal = true;
        bool firstPass = true;
        constexpr int kBlurPassCount = 6;

        m_blurShader->use();
        for (int pass = 0; pass < kBlurPassCount; ++pass)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_pingPongFramebufferIds[horizontal ? 1 : 0]);
            m_blurShader->setInt("uHorizontal", horizontal ? 1 : 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, firstPass ? m_compositeBrightTextureId
                                                   : m_pingPongTextureIds[horizontal ? 0 : 1]);
            m_blurShader->setInt("uSourceTexture", 0);
            m_fullScreenPass.draw();
            horizontal = !horizontal;
            if (firstPass)
            {
                firstPass = false;
            }
        }

        activeBloomTextureId = m_pingPongTextureIds[horizontal ? 0 : 1];
    }

    if (debugView.postDebugViewMode == static_cast<int>(PostDebugViewMode::FinalImage))
    {
        if (!debugView.postProcessingEnabled)
        {
            presentSceneTexture();
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, m_width, m_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_tonemapShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_compositeColorTextureId);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, activeBloomTextureId);
        m_tonemapShader->setInt("uSceneTexture", 0);
        m_tonemapShader->setInt("uBloomTexture", 1);
        m_tonemapShader->setFloat("uExposure", settings.exposure);
        m_tonemapShader->setFloat("uBloomIntensity", settings.bloomIntensity);
        m_tonemapShader->setFloat("uGamma", settings.gamma);
        m_tonemapShader->setFloat("uContrast", settings.contrast);
        m_tonemapShader->setFloat("uVignetteStrength", settings.vignetteStrength);
        m_tonemapShader->setFloat("uSaturation", settings.saturation);
        m_tonemapShader->setFloat("uMidtoneLift", settings.midtoneLift);
        m_tonemapShader->setFloat("uToneMappingEnabled",
                                  debugView.toneMappingEnabled ? 1.0f : 0.0f);
        m_tonemapShader->setFloat("uPostDebugViewMode",
                                  static_cast<float>(PostDebugViewMode::FinalImage));
        m_tonemapShader->setFloat("uDebugExposureScale", debugExposureScale(settings.exposure));
        m_fullScreenPass.draw();
        return;
    }

    presentDebugView(settings, debugView, activeBloomTextureId);
}

unsigned int PostProcessor::sceneTextureId() const noexcept
{
    return m_sceneColorTextureId;
}

unsigned int PostProcessor::sceneDepthTextureId() const noexcept
{
    return m_sceneDepthTextureId;
}

unsigned int PostProcessor::sceneLightingTextureId() const noexcept
{
    return m_sceneLightingTextureId;
}

void PostProcessor::presentSceneTexture() const
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_compositeFramebufferId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::presentDebugView(const PostProcessSettings& settings,
                                     const DebugViewSettings& debugView,
                                     unsigned int bloomTextureId) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_tonemapShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_compositeColorTextureId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomTextureId);
    m_tonemapShader->setInt("uSceneTexture", 0);
    m_tonemapShader->setInt("uBloomTexture", 1);
    m_tonemapShader->setFloat("uExposure", settings.exposure);
    m_tonemapShader->setFloat("uBloomIntensity", settings.bloomIntensity);
    m_tonemapShader->setFloat("uGamma", settings.gamma);
    m_tonemapShader->setFloat("uContrast", settings.contrast);
    m_tonemapShader->setFloat("uVignetteStrength", settings.vignetteStrength);
    m_tonemapShader->setFloat("uSaturation", settings.saturation);
    m_tonemapShader->setFloat("uMidtoneLift", settings.midtoneLift);
    m_tonemapShader->setFloat("uToneMappingEnabled", debugView.toneMappingEnabled ? 1.0f : 0.0f);
    m_tonemapShader->setFloat("uPostDebugViewMode",
                              static_cast<float>(debugView.postDebugViewMode));
    m_tonemapShader->setFloat("uDebugExposureScale", debugExposureScale(settings.exposure));
    m_fullScreenPass.draw();
}

void PostProcessor::createBuffers(int width, int height)
{
    m_width = width;
    m_height = height;

    glGenFramebuffers(1, &m_sceneFramebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFramebufferId);

    glGenTextures(1, &m_sceneColorTextureId);
    attachColorTexture(m_sceneColorTextureId, width, height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_sceneColorTextureId, 0);

    glGenTextures(1, &m_sceneLightingTextureId);
    attachColorTexture(m_sceneLightingTextureId, width, height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                           m_sceneLightingTextureId, 0);

    const unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    glGenTextures(1, &m_sceneDepthTextureId);
    glBindTexture(GL_TEXTURE_2D, m_sceneDepthTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL,
                 GL_UNSIGNED_INT_24_8, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                           m_sceneDepthTextureId, 0);
    validateFramebuffer(m_sceneFramebufferId, "scene hdr");

    glGenFramebuffers(1, &m_compositeFramebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_compositeFramebufferId);

    glGenTextures(1, &m_compositeColorTextureId);
    attachColorTexture(m_compositeColorTextureId, width, height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_compositeColorTextureId, 0);

    glGenTextures(1, &m_compositeBrightTextureId);
    attachColorTexture(m_compositeBrightTextureId, width, height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                           m_compositeBrightTextureId, 0);

    glDrawBuffers(2, attachments);
    validateFramebuffer(m_compositeFramebufferId, "lighting composite");

    glGenFramebuffers(2, m_pingPongFramebufferIds);
    glGenTextures(2, m_pingPongTextureIds);

    for (int index = 0; index < 2; ++index)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingPongFramebufferIds[index]);
        attachColorTexture(m_pingPongTextureIds[index], width, height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               m_pingPongTextureIds[index], 0);
        validateFramebuffer(m_pingPongFramebufferIds[index], "ping pong");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::destroyBuffers() noexcept
{
    glDeleteTextures(1, &m_sceneColorTextureId);
    glDeleteTextures(1, &m_sceneLightingTextureId);
    glDeleteTextures(1, &m_sceneDepthTextureId);
    glDeleteFramebuffers(1, &m_sceneFramebufferId);
    glDeleteTextures(1, &m_compositeColorTextureId);
    glDeleteTextures(1, &m_compositeBrightTextureId);
    glDeleteFramebuffers(1, &m_compositeFramebufferId);
    glDeleteTextures(2, m_pingPongTextureIds);
    glDeleteFramebuffers(2, m_pingPongFramebufferIds);

    m_sceneFramebufferId = 0;
    m_sceneColorTextureId = 0;
    m_sceneLightingTextureId = 0;
    m_sceneDepthTextureId = 0;
    m_compositeFramebufferId = 0;
    m_compositeColorTextureId = 0;
    m_compositeBrightTextureId = 0;
    m_pingPongFramebufferIds[0] = 0;
    m_pingPongFramebufferIds[1] = 0;
    m_pingPongTextureIds[0] = 0;
    m_pingPongTextureIds[1] = 0;
}
} // namespace engine
