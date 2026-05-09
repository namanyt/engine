#pragma once

#include "runtime/RuntimeIds.h"
#include "runtime/RuntimeMode.h"
#include "runtime/StartupFlowOverlay.h"
#include "runtime/VnScript.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace engine
{
class AssetManager;

class VNRuntime final : public RuntimeMode
{
  public:
    explicit VNRuntime(
        std::filesystem::path scriptAssetPath = std::filesystem::path{"scripts/test.vnscript"},
        RuntimeId returnRuntimeId = RuntimeId::DaylightSandbox);
    ~VNRuntime() override;

    const char* name() const override;
    void prepareActivation(ActivationContext& activationContext) override;
    void activate(ActivationContext& activationContext) override;
    void deactivate(Renderer& renderer) override;
    void update(const UpdateContext& updateContext) override;
    void render(const RenderContext& renderContext) override;
    bool canRenderLoadingPreview() const override;
    void renderLoadingPreview(const RenderContext& renderContext) override;

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    void drawDebugUi(const DebugUiContext& debugUiContext) override;
#endif

  private:
    struct CharacterDefinition final
    {
        std::filesystem::path assetPath;
    };

    struct StagedCharacter final
    {
        std::string identifier;
        VnStageRegion stageRegion = VnStageRegion::Center;
        float xOffsetNormalized = 0.0f;
        float yOffsetNormalized = 0.0f;
        float scale = 1.0f;
    };

    struct SceneState final
    {
        std::filesystem::path backgroundAssetPath;
        std::string speakerName;
        std::string dialogueText;
        std::unordered_map<std::string, CharacterDefinition> characterDefinitions;
        std::vector<StagedCharacter> stagedCharacters;
    };

    bool advanceRequested(const RawInputState& inputState) const;
    void executeUntilBlocked();
    void executeInstruction(const VnInstruction& instruction);
    void finalizeFadeIfComplete();
    void commitVisualState(bool useFade, float fadeDurationSeconds);
    bool updateTypewriter(float deltaSeconds);
    void resetTypewriter();
    void completeTypewriter();
    bool isCurrentLineFullyRevealed() const;
    std::string visibleDialogueText() const;
    float punctuationPauseSeconds(std::size_t revealedCharacterCount) const;
    StartupFlowOverlay buildSceneOverlay() const;
    std::filesystem::path resolveTextureAssetPath(const std::filesystem::path& assetPath) const;
    float stageAnchorXNormalized(VnStageRegion stageRegion) const;
    void applyOverlayTextures(Renderer& renderer) const;

    std::filesystem::path m_scriptAssetPath;
    std::filesystem::path m_resolvedScriptPath;
    RuntimeId m_returnRuntimeId = RuntimeId::DaylightSandbox;
    std::shared_ptr<AssetManager> m_assetManager;
    VnScript m_script;
    SceneState m_sceneState;
    StartupFlowOverlay m_activeOverlay;
    StartupFlowOverlay m_transitionSourceOverlay;
    StartupFlowOverlay m_transitionTargetOverlay;
    std::size_t m_nextInstructionIndex = 0;
    std::size_t m_visibleDialogueCharacterCount = 0;
    float m_waitRemainingSeconds = 0.0f;
    float m_transitionElapsedSeconds = 0.0f;
    float m_transitionDurationSeconds = 0.0f;
    float m_typewriterCarrySeconds = 0.0f;
    float m_typewriterPauseRemainingSeconds = 0.0f;
    bool m_waitingForAdvance = false;
    bool m_sceneDirty = false;
    bool m_prepared = false;
    bool m_finished = false;
};
} // namespace engine
