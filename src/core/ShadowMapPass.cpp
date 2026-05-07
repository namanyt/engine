#include "core/ShadowMapPass.h"

#include "core/RenderDebug.h"
#include "core/Shader.h"
#include "graphics/Mesh.h"
#include "math/Transform.h"

#include <glad/glad.h>

#include <stdexcept>

namespace engine
{
ShadowMapPass::ShadowMapPass(const std::shared_ptr<ShaderLibrary>& shaderLibrary)
    : m_shaderLibrary(shaderLibrary)
{
    m_shadowShader = std::make_unique<Shader>(m_shaderLibrary->shaderPath("shadow_depth.vert"),
                                              m_shaderLibrary->shaderPath("shadow_depth.frag"));
    createResources(m_size);
}

ShadowMapPass::~ShadowMapPass()
{
    destroyResources();
}

void ShadowMapPass::resize(int size)
{
    const int safeSize = size > 0 ? size : 1;
    if (safeSize == m_size)
    {
        return;
    }

    destroyResources();
    createResources(safeSize);
}

void ShadowMapPass::begin(const Mat4& lightViewProjection) const
{
    pushRenderDebugGroup("Shadow Pass");
    glViewport(0, 0, m_size, m_size);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
    glClear(GL_DEPTH_BUFFER_BIT);
    m_shadowShader->use();
    m_shadowShader->setMat4("uLightViewProjection", lightViewProjection);
}

void ShadowMapPass::draw(const Mesh& mesh, const Transform& transform) const
{
    draw(mesh, transform.modelMatrix());
}

void ShadowMapPass::draw(const Mesh& mesh, const Mat4& modelMatrix) const
{
    m_shadowShader->setMat4("uModel", modelMatrix);
    mesh.draw();
}

void ShadowMapPass::end(int viewportWidth, int viewportHeight) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportWidth, viewportHeight);
    popRenderDebugGroup();
}

unsigned int ShadowMapPass::depthTextureId() const noexcept
{
    return m_depthTextureId;
}

void ShadowMapPass::createResources(int size)
{
    m_size = size;

    glGenFramebuffers(1, &m_framebufferId);
    glGenTextures(1, &m_depthTextureId);

    labelGlObject(GL_FRAMEBUFFER, m_framebufferId, "ShadowPass.Framebuffer");
    labelGlObject(GL_TEXTURE, m_depthTextureId, "ShadowPass.DepthTexture");

    glBindTexture(GL_TEXTURE_2D, m_depthTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_size, m_size, 0, GL_DEPTH_COMPONENT,
                 GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    constexpr float kBorderColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, kBorderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTextureId, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Shadow framebuffer is incomplete.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMapPass::destroyResources() noexcept
{
    glDeleteTextures(1, &m_depthTextureId);
    glDeleteFramebuffers(1, &m_framebufferId);
    m_depthTextureId = 0;
    m_framebufferId = 0;
}
} // namespace engine
