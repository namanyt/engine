#pragma once

#include "core/FullScreenPass.h"
#include "core/ShaderLibrary.h"
#include "world/Lighting.h"

#include <memory>

namespace engine
{
class Shader;

class PostProcessor final
{
  public:
    explicit PostProcessor(const std::shared_ptr<ShaderLibrary>& shaderLibrary);
    ~PostProcessor();

    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;
    PostProcessor(PostProcessor&&) = delete;
    PostProcessor& operator=(PostProcessor&&) = delete;

    void resize(int width, int height);
    void beginScene() const;
    void composeLighting(unsigned int atmosphereTextureId, float bloomThreshold) const;
    void endScene(const PostProcessSettings& settings, const DebugViewSettings& debugView) const;
    void presentSceneTexture() const;
    void presentDebugView(const PostProcessSettings& settings, const DebugViewSettings& debugView,
                          unsigned int bloomTextureId) const;

    unsigned int sceneTextureId() const noexcept;
    unsigned int sceneDepthTextureId() const noexcept;

  private:
    void createBuffers(int width, int height);
    void destroyBuffers() noexcept;

    int m_width = 1;
    int m_height = 1;
    unsigned int m_sceneFramebufferId = 0;
    unsigned int m_sceneColorTextureId = 0;
    unsigned int m_sceneDepthTextureId = 0;
    unsigned int m_compositeFramebufferId = 0;
    unsigned int m_compositeColorTextureId = 0;
    unsigned int m_compositeBrightTextureId = 0;
    unsigned int m_pingPongFramebufferIds[2] = {0, 0};
    unsigned int m_pingPongTextureIds[2] = {0, 0};
    FullScreenPass m_fullScreenPass;
    std::shared_ptr<ShaderLibrary> m_shaderLibrary;
    std::unique_ptr<Shader> m_blurShader;
    std::unique_ptr<Shader> m_compositionShader;
    std::unique_ptr<Shader> m_tonemapShader;
};
} // namespace engine
