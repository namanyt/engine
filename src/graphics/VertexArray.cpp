#include "graphics/VertexArray.h"

#include <glad/glad.h>

#include <stdexcept>

namespace engine
{
VertexArray::VertexArray()
{
    glGenVertexArrays(1, &m_arrayId);
    if (m_arrayId == 0)
    {
        throw std::runtime_error("Failed to create an OpenGL vertex array.");
    }
}

VertexArray::~VertexArray()
{
    if (m_arrayId != 0)
    {
        glDeleteVertexArrays(1, &m_arrayId);
    }
}

void VertexArray::bind() const
{
    glBindVertexArray(m_arrayId);
}

void VertexArray::unbind()
{
    glBindVertexArray(0);
}

void VertexArray::setAttribute(
    unsigned int index,
    int componentCount,
    unsigned int type,
    bool normalized,
    std::size_t stride,
    std::size_t offset) const
{
    glVertexAttribPointer(
        index,
        componentCount,
        type,
        normalized ? GL_TRUE : GL_FALSE,
        static_cast<GLsizei>(stride),
        reinterpret_cast<const void*>(offset));
    glEnableVertexAttribArray(index);
}
} // namespace engine
