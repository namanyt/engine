#pragma once

#include <cstddef>

namespace engine
{
class VertexBuffer final
{
public:
    VertexBuffer(const void* data, std::size_t sizeBytes);
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&&) = delete;
    VertexBuffer& operator=(VertexBuffer&&) = delete;

    void bind() const;
    static void unbind();

private:
    unsigned int m_bufferId = 0;
};
} // namespace engine
