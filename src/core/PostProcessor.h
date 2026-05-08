#pragma once

#include "core/FullScreenPass.h"
#include "core/RuntimeOverlay.h"
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

    void prepareOverlayResources();
    void prepareWorldResources();
    void resize(int width, int height);
    void beginScene() const;
    void composeLighting(unsigned int atmosphereTextureId, float bloomThreshold) const;
    void endScene(const PostProcessSettings& settings, const DebugViewSettings& debugView) const;
    void endOverlayScene() const;
    void setRuntimeOverlayTexture(unsigned int textureId, int width, int height) noexcept;
    void setRuntimeOverlayTexture(unsigned int textureId, int width, int height,
                                  const RuntimeOverlayOptions& options) noexcept;
    void presentSceneTexture() const;
    void presentDebugView(const PostProcessSettings& settings, const DebugViewSettings& debugView,
                          unsigned int bloomTextureId) const;

    unsigned int sceneTextureId() const noexcept;
    unsigned int sceneDepthTextureId() const noexcept;

  private:
    void createBuffers(int width, int height);
    void destroyBuffers() noexcept;
    void ensureOverlayShader() const;
    void ensureWorldShaders() const;
    void drawRuntimeOverlay() const;

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
    mutable const Shader* m_blurShader = nullptr;
    mutable const Shader* m_compositionShader = nullptr;
    mutable const Shader* m_tonemapShader = nullptr;
    mutable const Shader* m_overlayShader = nullptr;
    unsigned int m_runtimeOverlayTextureId = 0;
    int m_runtimeOverlayTextureWidth = 0;
    int m_runtimeOverlayTextureHeight = 0;
    RuntimeOverlayOptions m_runtimeOverlayOptions{};
};
} // namespace engine
