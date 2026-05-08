#include "core/PostProcessor.h"

#include "core/RenderDebug.h"
#include "core/Shader.h"

#include <glad/glad.h>

#include <algorithm>
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
    createBuffers(m_width, m_height);
}

PostProcessor::~PostProcessor()
{
    destroyBuffers();
}

void PostProcessor::prepareOverlayResources()
{
    ensureOverlayShader();
}

void PostProcessor::prepareWorldResources()
{
    ensureWorldShaders();
    ensureOverlayShader();
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
    ensureWorldShaders();
    ScopedRenderDebugGroup lightingScope("Lighting Pass");
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
    ensureWorldShaders();
    ScopedRenderDebugGroup postScope("Post Processing Pass");
    unsigned int activeBloomTextureId = m_compositeBrightTextureId;

    if (debugView.postProcessingEnabled)
    {
        ScopedRenderDebugGroup bloomScope("Bloom Pass");
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
            drawRuntimeOverlay();
            return;
        }

        ScopedRenderDebugGroup toneMapScope("Tone Mapping Pass");
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
        drawRuntimeOverlay();
        return;
    }

    presentDebugView(settings, debugView, activeBloomTextureId);
    drawRuntimeOverlay();
}

void PostProcessor::endOverlayScene() const
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_sceneFramebufferId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    drawRuntimeOverlay();
}

void PostProcessor::setRuntimeOverlayTexture(unsigned int textureId, int width, int height) noexcept
{
    m_runtimeOverlayTextureId = textureId;
    m_runtimeOverlayTextureWidth = width;
    m_runtimeOverlayTextureHeight = height;
}

unsigned int PostProcessor::sceneTextureId() const noexcept
{
    return m_sceneColorTextureId;
}

unsigned int PostProcessor::sceneDepthTextureId() const noexcept
{
    return m_sceneDepthTextureId;
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
    ensureWorldShaders();
    ScopedRenderDebugGroup toneMapScope("Tone Mapping Pass");
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

void PostProcessor::drawRuntimeOverlay() const
{
    ensureOverlayShader();
    if (m_overlayShader == nullptr || m_runtimeOverlayTextureId == 0 ||
        m_runtimeOverlayTextureWidth <= 0 || m_runtimeOverlayTextureHeight <= 0)
    {
        return;
    }

    constexpr float kMarginPixels = 24.0f;
    constexpr float kMaxScreenFraction = 0.24f;
    constexpr float kMaxWidthPixels = 320.0f;
    constexpr float kMaxHeightPixels = 240.0f;

    const float textureAspect = static_cast<float>(m_runtimeOverlayTextureWidth) /
                                static_cast<float>(std::max(m_runtimeOverlayTextureHeight, 1));
    const float maxOverlayWidth =
        std::min(static_cast<float>(m_width) * kMaxScreenFraction, kMaxWidthPixels);
    const float maxOverlayHeight =
        std::min(static_cast<float>(m_height) * kMaxScreenFraction, kMaxHeightPixels);

    float overlayWidth = maxOverlayWidth;
    float overlayHeight = overlayWidth / std::max(textureAspect, 0.0001f);
    if (overlayHeight > maxOverlayHeight)
    {
        overlayHeight = maxOverlayHeight;
        overlayWidth = overlayHeight * textureAspect;
    }

    if (overlayWidth <= 1.0f || overlayHeight <= 1.0f)
    {
        return;
    }

    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_overlayShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_runtimeOverlayTextureId);
    m_overlayShader->setInt("uOverlayTexture", 0);
    m_overlayShader->setVec2("uScreenSize", static_cast<float>(m_width),
                             static_cast<float>(m_height));
    m_overlayShader->setVec2("uOverlaySizePixels", overlayWidth, overlayHeight);
    m_overlayShader->setVec2("uOverlayMarginPixels", kMarginPixels, kMarginPixels);
    m_overlayShader->setFloat("uOverlayOpacity", 1.0f);
    m_fullScreenPass.draw();
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!blendWasEnabled)
    {
        glDisable(GL_BLEND);
    }

    if (depthTestWasEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
}

void PostProcessor::ensureOverlayShader() const
{
    if (m_overlayShader != nullptr)
    {
        return;
    }

    m_overlayShader = &m_shaderLibrary->loadGraphicsProgram(
        "renderer.post.overlay", std::filesystem::path("post_tonemap.vert"),
        std::filesystem::path("ui_overlay.frag"));
}

void PostProcessor::ensureWorldShaders() const
{
    if (m_blurShader == nullptr)
    {
        m_blurShader = &m_shaderLibrary->loadGraphicsProgram(
            "renderer.post.blur", std::filesystem::path("post_blur.vert"),
            std::filesystem::path("post_blur.frag"));
    }

    if (m_compositionShader == nullptr)
    {
        m_compositionShader = &m_shaderLibrary->loadGraphicsProgram(
            "renderer.post.compose", std::filesystem::path("post_tonemap.vert"),
            std::filesystem::path("post_compose.frag"));
    }

    if (m_tonemapShader == nullptr)
    {
        m_tonemapShader = &m_shaderLibrary->loadGraphicsProgram(
            "renderer.post.tonemap", std::filesystem::path("post_tonemap.vert"),
            std::filesystem::path("post_tonemap.frag"));
    }
}

void PostProcessor::createBuffers(int width, int height)
{
    m_width = width;
    m_height = height;

    glGenFramebuffers(1, &m_sceneFramebufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFramebufferId);

    labelGlObject(GL_FRAMEBUFFER, m_sceneFramebufferId, "Scene.Framebuffer");

    glGenTextures(1, &m_sceneColorTextureId);
    attachColorTexture(m_sceneColorTextureId, width, height);
    labelGlObject(GL_TEXTURE, m_sceneColorTextureId, "Scene.Color");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_sceneColorTextureId, 0);

    const unsigned int sceneAttachment = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &sceneAttachment);

    glGenTextures(1, &m_sceneDepthTextureId);
    glBindTexture(GL_TEXTURE_2D, m_sceneDepthTextureId);
    labelGlObject(GL_TEXTURE, m_sceneDepthTextureId, "Scene.Depth");
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

    labelGlObject(GL_FRAMEBUFFER, m_compositeFramebufferId, "Composite.Framebuffer");

    glGenTextures(1, &m_compositeColorTextureId);
    attachColorTexture(m_compositeColorTextureId, width, height);
    labelGlObject(GL_TEXTURE, m_compositeColorTextureId, "Composite.Color");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           m_compositeColorTextureId, 0);

    glGenTextures(1, &m_compositeBrightTextureId);
    attachColorTexture(m_compositeBrightTextureId, width, height);
    labelGlObject(GL_TEXTURE, m_compositeBrightTextureId, "Composite.BloomExtract");
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                           m_compositeBrightTextureId, 0);

    const unsigned int compositeAttachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, compositeAttachments);
    validateFramebuffer(m_compositeFramebufferId, "lighting composite");

    glGenFramebuffers(2, m_pingPongFramebufferIds);
    glGenTextures(2, m_pingPongTextureIds);

    labelGlObject(GL_FRAMEBUFFER, m_pingPongFramebufferIds[0], "Bloom.PingFramebuffer");
    labelGlObject(GL_FRAMEBUFFER, m_pingPongFramebufferIds[1], "Bloom.PongFramebuffer");
    labelGlObject(GL_TEXTURE, m_pingPongTextureIds[0], "Bloom.PingTexture");
    labelGlObject(GL_TEXTURE, m_pingPongTextureIds[1], "Bloom.PongTexture");

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
    glDeleteTextures(1, &m_sceneDepthTextureId);
    glDeleteFramebuffers(1, &m_sceneFramebufferId);
    glDeleteTextures(1, &m_compositeColorTextureId);
    glDeleteTextures(1, &m_compositeBrightTextureId);
    glDeleteFramebuffers(1, &m_compositeFramebufferId);
    glDeleteTextures(2, m_pingPongTextureIds);
    glDeleteFramebuffers(2, m_pingPongFramebufferIds);

    m_sceneFramebufferId = 0;
    m_sceneColorTextureId = 0;
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
