#pragma once

#include "math/Transform.h"

namespace engine
{
class Mesh;
class Shader;

class Renderer final
{
public:
    Renderer();
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void setViewport(int width, int height);
    void beginFrame() const;
    void draw(
        const Mesh& mesh,
        const Shader& shader,
        const Transform& transform,
        float timeSeconds,
        float animationType,
        float timeOffset) const;

private:
    void updateProjectionMatrix();

    int m_framebufferWidth = 800;
    int m_framebufferHeight = 600;
    Vec3 m_viewPosition{0.0f, 0.0f, 4.0f};
    Mat4 m_viewMatrix;
    Mat4 m_projectionMatrix;
};
} // namespace engine
