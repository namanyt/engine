#pragma once

#include "core/ShaderLibrary.h"
#include "math/Types.h"

#include <memory>

namespace engine
{
class Mesh;
class Shader;
struct Transform;

class ShadowMapPass final
{
  public:
    explicit ShadowMapPass(const std::shared_ptr<ShaderLibrary>& shaderLibrary);
    ~ShadowMapPass();

    ShadowMapPass(const ShadowMapPass&) = delete;
    ShadowMapPass& operator=(const ShadowMapPass&) = delete;
    ShadowMapPass(ShadowMapPass&&) = delete;
    ShadowMapPass& operator=(ShadowMapPass&&) = delete;

    void resize(int size);
    void begin(const Mat4& lightViewProjection) const;
    void draw(const Mesh& mesh, const Mat4& modelMatrix) const;
    void draw(const Mesh& mesh, const Transform& transform) const;
    void end(int viewportWidth, int viewportHeight) const;

    unsigned int depthTextureId() const noexcept;

  private:
    void createResources(int size);
    void destroyResources() noexcept;

    int m_size = 1024;
    unsigned int m_framebufferId = 0;
    unsigned int m_depthTextureId = 0;
    std::shared_ptr<ShaderLibrary> m_shaderLibrary;
    std::unique_ptr<Shader> m_shadowShader;
    Mat4 m_lightViewProjection = Mat4::identity();
};
} // namespace engine
