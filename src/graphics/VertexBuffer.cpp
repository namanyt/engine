#include "graphics/VertexBuffer.h"

#include <glad/glad.h>

#include <stdexcept>

namespace engine
{
VertexBuffer::VertexBuffer(const void* data, std::size_t sizeBytes)
{
    glGenBuffers(1, &m_bufferId);
    if (m_bufferId == 0)
    {
        throw std::runtime_error("Failed to create an OpenGL vertex buffer.");
    }

    bind();
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeBytes), data, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer()
{
    if (m_bufferId != 0)
    {
        glDeleteBuffers(1, &m_bufferId);
    }
}

void VertexBuffer::bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferId);
}

void VertexBuffer::unbind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
} // namespace engine
