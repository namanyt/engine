#pragma once

#include "Shader.h"

#include <filesystem>

namespace engine
{
class Renderer final
{
public:
    explicit Renderer(const std::filesystem::path& shaderDirectory);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void render(float timeSeconds, int framebufferWidth, int framebufferHeight) const;

private:
    void initializeGeometry();

    unsigned int m_vertexArrayObject = 0;
    unsigned int m_vertexBufferObject = 0;
    Shader m_shader;
};
} // namespace engine
