#pragma once

#include <cstddef>

namespace engine
{
class IndexBuffer final
{
public:
    IndexBuffer(const unsigned int* data, std::size_t indexCount);
    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
    IndexBuffer(IndexBuffer&&) = delete;
    IndexBuffer& operator=(IndexBuffer&&) = delete;

    void bind() const;
    static void unbind();

    unsigned int count() const noexcept;

private:
    unsigned int m_bufferId = 0;
    unsigned int m_count = 0;
};
} // namespace engine
