#pragma once

#include "math/Types.h"
#include "runtime/RuntimeMode.h"

#include <memory>
#include <string>

namespace engine
{
class LoadingScene;
class SceneMetasset;

class LoadingRuntime final : public RuntimeMode
{
  public:
    LoadingRuntime(std::unique_ptr<RuntimeMode> nextRuntimeMode, float minimumDurationSeconds,
                   std::string nextRuntimeLabel);
    ~LoadingRuntime() override;

    const char* name() const override;
    void activate(ActivationContext& activationContext) override;
    void deactivate(Renderer& renderer) override;
    void update(const UpdateContext& updateContext) override;
    void render(const RenderContext& renderContext) override;

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    void drawDebugUi(const DebugUiContext& debugUiContext) override;
#endif

  private:
    Color activeClearColor(float timeSeconds) const;
    void applyOverlayTexture(Renderer& renderer) const;
    void setProgress(Application& application, float progress, const char* phase);
    void updateProgressTitle(Application& application) const;

    std::unique_ptr<SceneMetasset> m_sceneMetasset;
    std::unique_ptr<LoadingScene> m_scene;
    std::unique_ptr<RuntimeMode> m_nextRuntimeMode;
    std::string m_nextRuntimeLabel;
    std::string m_progressPhase;
    float m_activationProgress = 0.0f;
    float m_elapsedSeconds = 0.0f;
    float m_minimumDurationSeconds = 0.0f;
};
} // namespace engine
