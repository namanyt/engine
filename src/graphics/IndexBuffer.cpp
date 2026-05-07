#include "graphics/IndexBuffer.h"

#include "core/RenderDebug.h"

#include <glad/glad.h>

#include <stdexcept>

namespace engine
{
IndexBuffer::IndexBuffer(const unsigned int* data, std::size_t indexCount)
    : m_count(static_cast<unsigned int>(indexCount))
{
    glGenBuffers(1, &m_bufferId);
    if (m_bufferId == 0)
    {
        throw std::runtime_error("Failed to create an OpenGL index buffer.");
    }

    labelGlObject(GL_BUFFER, m_bufferId, "Geometry.IndexBuffer");

    bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indexCount * sizeof(unsigned int)), data, GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer()
{
    if (m_bufferId != 0)
    {
        glDeleteBuffers(1, &m_bufferId);
    }
}

void IndexBuffer::bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufferId);
}

void IndexBuffer::unbind()
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

unsigned int IndexBuffer::count() const noexcept
{
    return m_count;
}
} // namespace engine
