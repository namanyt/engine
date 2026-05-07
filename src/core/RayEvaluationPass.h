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
    void evaluate(unsigned int sceneDepthTextureId, const FrameUniforms& frameUniforms) const;
    unsigned int atmosphereTextureId() const noexcept;
    int renderWidth() const noexcept;
    int renderHeight() const noexcept;

  private:
    void createResources(int width, int height);
    void destroyResources() noexcept;
    void invalidateHistory() noexcept;

    int m_fullWidth = 1;
    int m_fullHeight = 1;
    int m_halfWidth = 1;
    int m_halfHeight = 1;
    unsigned int m_halfFramebufferIds[2] = {0, 0};
    unsigned int m_historyTextureIds[2] = {0, 0};
    unsigned int m_halfDepthTextureIds[2] = {0, 0};
    unsigned int m_halfTemporalTextureIds[2] = {0, 0};
    unsigned int m_halfMetricsTextureId = 0;
    unsigned int m_halfAuxTextureId = 0;
    unsigned int m_resolveFramebufferId = 0;
    unsigned int m_atmosphereTextureId = 0;
    FullScreenPass m_fullScreenPass;
    std::shared_ptr<ShaderLibrary> m_shaderLibrary;
    std::unique_ptr<Shader> m_evaluateShader;
    std::unique_ptr<Shader> m_resolveShader;
    mutable unsigned int m_historyWriteIndex = 0;
    mutable bool m_historyValid = false;
};
} // namespace engine
