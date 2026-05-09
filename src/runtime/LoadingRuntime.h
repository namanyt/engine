#pragma once

#include "math/Types.h"
#include "runtime/RuntimeMode.h"
#include "runtime/StartupFlowOverlay.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace engine
{
class Application;
class AssetManager;
class ShaderLibrary;

class LoadingRuntime final : public RuntimeMode
{
  public:
    LoadingRuntime(std::unique_ptr<RuntimeMode> nextRuntimeMode, float minimumDurationSeconds,
                   std::string nextRuntimeLabel, LoadingScreenStyle loadingScreenStyle);
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
    struct BootSequenceEvent final
    {
        enum class Type
        {
            Line,
            Cursor,
            Status,
        };

        Type type = Type::Line;
        float timeSeconds = 0.0f;
        float durationSeconds = 0.0f;
        std::string text;
        Color color = Color::white();
        bool hasColor = false;
    };

    Color activeClearColor() const;
    void applyOverlayTexture(Renderer& renderer) const;
    void beginDeferredLoad(const UpdateContext& updateContext);
    void loadBootSequenceScript();
    BootSequenceTextEntry bootSequenceStatusText() const;
    float bootSequencePresentationEndTimeSeconds() const;
    int bootSequenceVisibleLineCount() const;
    bool bootSequenceCursorVisible() const;
    std::vector<BootSequenceTextEntry> visibleBootSequenceLines() const;
    float bootSequenceFadeStartTimeSeconds() const;
    float overlayOpacity() const;
    bool shouldRenderPreparedPreview() const;
    void refreshBootSequenceOverlayIfNeeded();
    float transitionReadyTimeSeconds() const;
    void setProgress(Application& application, float progress, const char* phase);
    void refreshOverlay();
    void updateProgressTitle(Application& application) const;

    std::shared_ptr<AssetManager> m_assetManager;
    ShaderLibrary* m_shaderLibrary = nullptr;
    std::filesystem::path m_assetRootDirectory;
    std::filesystem::path m_shaderDirectory;
    StartupFlowOverlay m_loadingOverlay;
    std::unique_ptr<RuntimeMode> m_nextRuntimeMode;
    std::string m_nextRuntimeLabel;
    std::string m_progressPhase;
    LoadingScreenStyle m_loadingScreenStyle = LoadingScreenStyle::ProgressOnly;
    float m_activationProgress = 0.0f;
    float m_elapsedSeconds = 0.0f;
    float m_minimumDurationSeconds = 0.0f;
    float m_loadingCompletedAtSeconds = -1.0f;
    float m_bootSequenceScriptDurationSeconds = 0.0f;
    int m_bootSequenceVisibleLineRevision = -1;
    bool m_bootSequenceCursorRevision = false;
    BootSequenceTextEntry m_bootSequenceStatusRevision;
    bool m_loadingStarted = false;
    bool m_transitionQueued = false;
    std::vector<BootSequenceEvent> m_bootSequenceEvents;
};
} // namespace engine
