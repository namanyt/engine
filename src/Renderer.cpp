#include "Renderer.h"

#include <glad/glad.h>

#include <array>
#include <stdexcept>

namespace engine
{
Renderer::Renderer(const std::filesystem::path& shaderDirectory)
    : m_shader(shaderDirectory / "vertex.glsl", shaderDirectory / "fragment.glsl")
{
    initializeGeometry();
}

Renderer::~Renderer()
{
    if (m_vertexBufferObject != 0)
    {
        glDeleteBuffers(1, &m_vertexBufferObject);
    }

    if (m_vertexArrayObject != 0)
    {
        glDeleteVertexArrays(1, &m_vertexArrayObject);
    }
}

void Renderer::render(float timeSeconds, int framebufferWidth, int framebufferHeight) const
{
    glClearColor(0.04f, 0.06f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_shader.use();
    m_shader.setFloat("uTime", timeSeconds);
    m_shader.setVec2("uResolution", static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight));

    glBindVertexArray(m_vertexArrayObject);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void Renderer::initializeGeometry()
{
    constexpr std::array<float, 9> triangleVertices = {
        -0.60f, -0.45f, 0.0f,
         0.60f, -0.45f, 0.0f,
         0.00f,  0.65f, 0.0f
    };

    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);

    if (m_vertexArrayObject == 0 || m_vertexBufferObject == 0)
    {
        throw std::runtime_error("Failed to allocate OpenGL geometry buffers.");
    }

    glBindVertexArray(m_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(triangleVertices.size() * sizeof(float)),
        triangleVertices.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * static_cast<GLsizei>(sizeof(float)), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
} // namespace engine
