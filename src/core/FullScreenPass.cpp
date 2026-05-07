#include "core/FullScreenPass.h"

#include <glad/glad.h>

#include <stdexcept>

namespace engine
{
FullScreenPass::FullScreenPass()
{
    constexpr float kVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
    };

    glGenVertexArrays(1, &m_vertexArrayId);
    glGenBuffers(1, &m_vertexBufferId);

    if (m_vertexArrayId == 0 || m_vertexBufferId == 0)
    {
        throw std::runtime_error("Failed to create full-screen pass resources.");
    }

    glBindVertexArray(m_vertexArrayId);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<const void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

FullScreenPass::~FullScreenPass()
{
    if (m_vertexBufferId != 0)
    {
        glDeleteBuffers(1, &m_vertexBufferId);
    }

    if (m_vertexArrayId != 0)
    {
        glDeleteVertexArrays(1, &m_vertexArrayId);
    }
}

void FullScreenPass::draw() const
{
    glBindVertexArray(m_vertexArrayId);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
} // namespace engine
