#pragma once

#include "core/FullScreenPass.h"
#include "core/ShaderLibrary.h"
#include "world/Lighting.h"

#include <memory>

namespace engine
{
class Shader;
struct FrameUniforms;

class RayEvaluationPass final
{
  public:
    explicit RayEvaluationPass(const std::shared_ptr<ShaderLibrary>& shaderLibrary);
    ~RayEvaluationPass();

    RayEvaluationPass(const RayEvaluationPass&) = delete;
    RayEvaluationPass& operator=(const RayEvaluationPass&) = delete;
    RayEvaluationPass(RayEvaluationPass&&) = delete;
    RayEvaluationPass& operator=(RayEvaluationPass&&) = delete;

    void resize(int width, int height);
    void evaluate(unsigned int sceneDepthTextureId, unsigned int sceneLightingTextureId,
                  const FrameUniforms& frameUniforms) const;
    unsigned int atmosphereTextureId() const noexcept;

  private:
    void createResources(int width, int height);
    void destroyResources() noexcept;

    int m_width = 1;
    int m_height = 1;
    unsigned int m_framebufferId = 0;
    unsigned int m_atmosphereTextureId = 0;
    FullScreenPass m_fullScreenPass;
    std::shared_ptr<ShaderLibrary> m_shaderLibrary;
    std::unique_ptr<Shader> m_computeShader;
};
} // namespace engine
