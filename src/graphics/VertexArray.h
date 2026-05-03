#pragma once

#include <cstddef>

namespace engine
{
class VertexArray final
{
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&&) = delete;
    VertexArray& operator=(VertexArray&&) = delete;

    void bind() const;
    static void unbind();

    void setAttribute(
        unsigned int index,
        int componentCount,
        unsigned int type,
        bool normalized,
        std::size_t stride,
        std::size_t offset) const;

private:
    unsigned int m_arrayId = 0;
};
} // namespace engine
