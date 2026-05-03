#include "core/Renderer.h"

#include "core/Shader.h"
#include "graphics/Mesh.h"

#include <glad/glad.h>

namespace
{
constexpr float kFieldOfViewRadians = 0.78539816339f;
constexpr float kNearPlane = 0.1f;
constexpr float kFarPlane = 100.0f;
} // namespace

namespace engine
{
Renderer::Renderer()
    : m_viewMatrix(makeLookAt(m_viewPosition, Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f}))
    , m_projectionMatrix(Mat4::identity())
{
    glEnable(GL_DEPTH_TEST);
    updateProjectionMatrix();
}

void Renderer::setViewport(int width, int height)
{
    m_framebufferWidth = width > 0 ? width : 1;
    m_framebufferHeight = height > 0 ? height : 1;

    glViewport(0, 0, m_framebufferWidth, m_framebufferHeight);
    updateProjectionMatrix();
}

void Renderer::beginFrame() const
{
    glClearColor(0.04f, 0.06f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::draw(
    const Mesh& mesh,
    const Shader& shader,
    const Transform& transform,
    float timeSeconds,
    float animationType,
    float timeOffset) const
{
    const Mat4 modelMatrix = transform.modelMatrix();

    shader.use();
    shader.setMat4("uModel", modelMatrix);
    shader.setMat4("uView", m_viewMatrix);
    shader.setMat4("uProjection", m_projectionMatrix);
    shader.setFloat("u_Time", timeSeconds);
    shader.setFloat("u_AnimationType", animationType);
    shader.setFloat("u_TimeOffset", timeOffset);
    shader.setVec3("uViewPosition", m_viewPosition.x, m_viewPosition.y, m_viewPosition.z);

    mesh.draw();
}

void Renderer::updateProjectionMatrix()
{
    const float aspectRatio = static_cast<float>(m_framebufferWidth) / static_cast<float>(m_framebufferHeight);
    m_projectionMatrix = makePerspective(kFieldOfViewRadians, aspectRatio, kNearPlane, kFarPlane);
}
} // namespace engine
