#include "runtime/VNRuntime.h"

#include "Application.h"
#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "core/Log.h"
#include "core/RenderPipeline.h"
#include "core/Renderer.h"
#include "runtime/OverlayUiLayout.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace engine
{
namespace
{
constexpr float kCharacterBaselineNormalized = 0.67f;
constexpr const char* kAdvancePrompt = "Click / Enter / Space";
constexpr float kAutoAdvanceDelaySeconds = 0.85f;

float typewriterSecondsPerCharacter(const float charactersPerSecond)
{
    return 1.0f / std::max(charactersPerSecond, 1.0f);
}

RuntimeOverlayOptions settingsContentOverlayOptions(const SettingsOverlayViewModel& viewModel)
{
    const SettingsPageModel page = buildSettingsPageModel(viewModel);
    RuntimeOverlayOptions options{};
    options.layout = RuntimeOverlayLayout::CustomPixels;
    options.opacity = 1.0f;
    options.minXPixels = page.chrome.contentBounds.left;
    options.minYPixels = page.chrome.contentBounds.top;
    options.widthPixels = page.chrome.contentBounds.right - page.chrome.contentBounds.left;
    options.heightPixels = page.chrome.contentBounds.bottom - page.chrome.contentBounds.top;
    return options;
}
} // namespace

VNRuntime::VNRuntime(std::filesystem::path scriptAssetPath, RuntimeId returnRuntimeId)
    : m_scriptAssetPath(std::move(scriptAssetPath)), m_returnRuntimeId(returnRuntimeId)
{
}

VNRuntime::~VNRuntime() = default;

const char* VNRuntime::name() const
{
    return "VNRuntime";
}

void VNRuntime::prepareActivation(ActivationContext& activationContext)
{
    if (m_prepared)
    {
        return;
    }

    if (activationContext.assetManager == nullptr)
    {
        throw std::runtime_error("VNRuntime activation requires an AssetManager.");
    }

    activationContext.renderer.prepareOverlayRenderingResources();
    m_assetManager = activationContext.assetManager;
    m_resolvedScriptPath = m_scriptAssetPath.is_absolute()
                               ? m_scriptAssetPath
                               : activationContext.assetRootDirectory / m_scriptAssetPath;
    m_script = parseVnScriptFile(m_resolvedScriptPath);
    m_sceneState = SceneState{};
    m_activeOverlay = StartupFlowOverlay::createSolid(0, 0, 0, 255);
    m_dialogueOverlay.reset();
    m_transitionSourceOverlay.reset();
    m_transitionTargetOverlay.reset();
    m_pauseIdleOverlay.reset();
    m_pauseResumeOverlay.reset();
    m_pauseSettingsOverlay.reset();
    m_pauseReturnOverlay.reset();
    m_settingsOverlayBaseTexture.reset();
    m_settingsOverlayContentTexture.reset();
    m_nextInstructionIndex = 0;
    m_visibleDialogueCharacterCount = 0;
    m_waitRemainingSeconds = 0.0f;
    m_transitionElapsedSeconds = 0.0f;
    m_transitionDurationSeconds = 0.0f;
    m_typewriterCarrySeconds = 0.0f;
    m_typewriterPauseRemainingSeconds = 0.0f;
    m_autoAdvanceRemainingSeconds = 0.0f;
    m_advanceReleaseRequired = false;
    m_autoAdvanceEnabled = false;
    m_waitingForAdvance = false;
    m_dialogueDirty = false;
    m_sceneDirty = false;
    m_finished = false;
    m_overlayView = OverlayView::None;
    m_pauseSelection = PauseMenuSelection::None;
    executeUntilBlocked();
    m_prepared = true;
}

void VNRuntime::activate(ActivationContext& activationContext)
{
    prepareActivation(activationContext);
    activationContext.application.setCursorCaptured(false);
    m_pauseIdleOverlay = StartupFlowOverlay::createPauseMenu(PauseMenuSelection::None);
    m_pauseResumeOverlay = StartupFlowOverlay::createPauseMenu(PauseMenuSelection::Resume);
    m_pauseSettingsOverlay = StartupFlowOverlay::createPauseMenu(PauseMenuSelection::Settings);
    m_pauseReturnOverlay =
        StartupFlowOverlay::createPauseMenu(PauseMenuSelection::ReturnToMainMenu);
    m_settingsOverlayBaseTexture.reset();
    m_settingsOverlayContentTexture.reset();
    m_overlayView = OverlayView::None;
    m_pauseSelection = PauseMenuSelection::None;
    applyOverlayTextures(activationContext.renderer);

    std::ostringstream stream;
    stream << "Activated VN runtime from script '" << m_resolvedScriptPath.string()
           << "' returning to " << runtimeDisplayName(m_returnRuntimeId) << '.';
    Log::info("VNRuntime", stream.str());
}

void VNRuntime::deactivate(Renderer& renderer)
{
    m_activeOverlay.reset();
    m_dialogueOverlay.reset();
    m_transitionSourceOverlay.reset();
    m_transitionTargetOverlay.reset();
    m_pauseIdleOverlay.reset();
    m_pauseResumeOverlay.reset();
    m_pauseSettingsOverlay.reset();
    m_pauseReturnOverlay.reset();
    m_settingsOverlayBaseTexture.reset();
    m_settingsOverlayContentTexture.reset();
    m_assetManager.reset();
    renderer.clearRuntimeOverlayTexture();
    renderer.clearSecondaryRuntimeOverlayTexture();
    m_overlayView = OverlayView::None;
    m_pauseSelection = PauseMenuSelection::None;
}

void VNRuntime::update(const UpdateContext& updateContext)
{
    if (m_finished)
    {
        return;
    }

    const Application::VnSettings& vnSettings = updateContext.application.vnSettings();
    m_autoAdvanceEnabled = vnSettings.autoAdvanceEnabled;

    if (m_overlayView != OverlayView::None)
    {
        handlePauseOverlayInput(updateContext);
        return;
    }

    if (updateContext.inputState.keyEscape.pressed)
    {
        m_overlayView = OverlayView::Pause;
        m_pauseSelection = PauseMenuSelection::None;
        updateContext.application.setCursorCaptured(false);
        return;
    }

    if (m_transitionDurationSeconds > 0.0f)
    {
        m_transitionElapsedSeconds += updateContext.deltaSeconds;
        finalizeFadeIfComplete();
        if (m_transitionDurationSeconds > 0.0f)
        {
            return;
        }
    }

    if (m_waitRemainingSeconds > 0.0f)
    {
        m_waitRemainingSeconds =
            std::max(0.0f, m_waitRemainingSeconds - updateContext.deltaSeconds);
        if (m_waitRemainingSeconds > 0.0f)
        {
            return;
        }
    }

    if (!advanceHeld(updateContext.inputState))
    {
        m_advanceReleaseRequired = false;
    }

    if (m_waitingForAdvance)
    {
        if (!isCurrentLineFullyRevealed())
        {
            if (!m_advanceReleaseRequired && advanceHeld(updateContext.inputState))
            {
                completeTypewriter();
                m_dialogueDirty = true;
                m_advanceReleaseRequired = true;
                commitVisualState(false, 0.0f);
                return;
            }

            if (updateTypewriter(updateContext.deltaSeconds, vnSettings.typingCharactersPerSecond))
            {
                m_dialogueDirty = true;
                commitVisualState(false, 0.0f);
            }

            return;
        }

        if (!m_advanceReleaseRequired && advanceRequested(updateContext.inputState))
        {
            m_waitingForAdvance = false;
            m_dialogueDirty = true;
            m_advanceReleaseRequired = true;
        }
        else if (m_autoAdvanceEnabled)
        {
            if (m_autoAdvanceRemainingSeconds <= 0.0f)
            {
                m_autoAdvanceRemainingSeconds = kAutoAdvanceDelaySeconds;
            }

            m_autoAdvanceRemainingSeconds =
                std::max(0.0f, m_autoAdvanceRemainingSeconds - updateContext.deltaSeconds);
            if (m_autoAdvanceRemainingSeconds > 0.0f)
            {
                return;
            }

            m_waitingForAdvance = false;
            m_dialogueDirty = true;
            m_advanceReleaseRequired = true;
        }
        else
        {
            if (m_advanceReleaseRequired || !advanceRequested(updateContext.inputState))
            {
                return;
            }

            m_waitingForAdvance = false;
            m_dialogueDirty = true;
            m_advanceReleaseRequired = true;
        }
    }

    executeUntilBlocked();
}

void VNRuntime::handlePauseOverlayInput(const UpdateContext& updateContext)
{
    if (m_overlayView == OverlayView::Settings)
    {
        SettingsOverlay::InputState settingsInput{};
        settingsInput.cancel = updateContext.inputState.keyEscape.pressed;
        settingsInput.click = updateContext.inputState.mouseLeft.pressed;
        settingsInput.mouseDown = updateContext.inputState.mouseLeft.down;
        settingsInput.mousePosition = updateContext.inputState.mousePosition;
        settingsInput.windowSize = updateContext.inputState.windowSize;

        if (m_settingsOverlay.update(settingsInput, updateContext.application) ==
            SettingsOverlay::Result::Close)
        {
            m_overlayView = OverlayView::Pause;
            m_pauseSelection = PauseMenuSelection::None;
        }

        const SettingsOverlayDirtyRegion dirtyRegions = m_settingsOverlay.consumeDirtyRegions();
        if (dirtyRegions != SettingsOverlayDirtyRegion::None)
        {
            refreshSettingsOverlay(dirtyRegions);
        }
        return;
    }

    bool hoveredPauseEntry = false;
    m_pauseSelection = PauseMenuSelection::None;
    if (updateContext.inputState.windowSize.x > 1.0f &&
        updateContext.inputState.windowSize.y > 1.0f)
    {
        const Vec2 designMouse = overlayui::toDesignSpace(updateContext.inputState.mousePosition,
                                                          updateContext.inputState.windowSize);
        if (overlayui::contains(overlayui::kPauseResumeRect, designMouse))
        {
            m_pauseSelection = PauseMenuSelection::Resume;
            hoveredPauseEntry = true;
        }
        else if (overlayui::contains(overlayui::kPauseSettingsRect, designMouse))
        {
            m_pauseSelection = PauseMenuSelection::Settings;
            hoveredPauseEntry = true;
        }
        else if (overlayui::contains(overlayui::kPauseReturnRect, designMouse))
        {
            m_pauseSelection = PauseMenuSelection::ReturnToMainMenu;
            hoveredPauseEntry = true;
        }
    }

    if (updateContext.inputState.keyEscape.pressed)
    {
        m_overlayView = OverlayView::None;
        return;
    }

    const bool clickSelection = updateContext.inputState.mouseLeft.pressed && hoveredPauseEntry;
    if (!clickSelection)
    {
        return;
    }

    if (m_pauseSelection == PauseMenuSelection::Resume)
    {
        m_overlayView = OverlayView::None;
        return;
    }

    if (m_pauseSelection == PauseMenuSelection::Settings)
    {
        m_overlayView = OverlayView::Settings;
        m_pauseSelection = PauseMenuSelection::None;
        m_settingsOverlay.activate(updateContext.application, true);
        refreshSettingsOverlay(SettingsOverlayDirtyRegion::Base |
                               SettingsOverlayDirtyRegion::Content);
        return;
    }

    RuntimeTransitionRequest request{};
    request.targetId = RuntimeId::Menu;
    request.minimumDurationSeconds = 1.1f;
    request.loadingLabel = "Main Menu";
    requestRuntimeChange(std::move(request));
}

void VNRuntime::render(const RenderContext& renderContext)
{
    applyOverlayTextures(renderContext.renderer);
    renderContext.renderer.setViewport(renderContext.framebufferWidth,
                                       renderContext.framebufferHeight);
    renderContext.renderPipeline.renderOverlayFrame(Color{0.0f, 0.0f, 0.0f, 1.0f});
}

bool VNRuntime::canRenderLoadingPreview() const
{
    return m_activeOverlay.valid() || m_transitionTargetOverlay.valid();
}

void VNRuntime::renderLoadingPreview(const RenderContext& renderContext)
{
    render(renderContext);
}

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
void VNRuntime::drawDebugUi(const DebugUiContext& debugUiContext)
{
    (void)debugUiContext;
}
#endif

bool VNRuntime::advanceRequested(const RawInputState& inputState) const
{
    return inputState.mouseLeft.pressed || inputState.keyEnter.pressed ||
           inputState.keySpace.pressed;
}

bool VNRuntime::advanceHeld(const RawInputState& inputState) const
{
    return inputState.mouseLeft.down || inputState.keyEnter.down || inputState.keySpace.down;
}

void VNRuntime::executeUntilBlocked()
{
    while (!m_finished && m_nextInstructionIndex < m_script.instructions.size() &&
           !m_waitingForAdvance && m_waitRemainingSeconds <= 0.0f &&
           m_transitionDurationSeconds <= 0.0f)
    {
        executeInstruction(m_script.instructions[m_nextInstructionIndex]);
        ++m_nextInstructionIndex;
    }
}

void VNRuntime::executeInstruction(const VnInstruction& instruction)
{
    switch (instruction.type)
    {
    case VnCommandType::Background:
        m_sceneState.backgroundAssetPath = instruction.assetPath;
        m_sceneDirty = true;
        return;
    case VnCommandType::CharacterSet:
        m_sceneState.characterDefinitions[instruction.identifier] =
            CharacterDefinition{instruction.assetPath};
        return;
    case VnCommandType::Character:
    {
        const auto definition = m_sceneState.characterDefinitions.find(instruction.identifier);
        if (definition == m_sceneState.characterDefinitions.end())
        {
            throw std::runtime_error("VNRuntime referenced undefined CHARACTER_SET id '" +
                                     instruction.identifier + "'.");
        }

        bool foundStagedCharacter = false;
        for (StagedCharacter& stagedCharacter : m_sceneState.stagedCharacters)
        {
            if (stagedCharacter.identifier != instruction.identifier)
            {
                continue;
            }

            stagedCharacter.stageRegion = instruction.stageRegion;
            stagedCharacter.xOffsetNormalized = instruction.xOffsetNormalized;
            stagedCharacter.yOffsetNormalized = instruction.yOffsetNormalized;
            stagedCharacter.scale = instruction.scale;
            foundStagedCharacter = true;
            break;
        }

        if (!foundStagedCharacter)
        {
            m_sceneState.stagedCharacters.push_back(StagedCharacter{
                instruction.identifier, instruction.stageRegion, instruction.xOffsetNormalized,
                instruction.yOffsetNormalized, instruction.scale});
        }

        m_sceneDirty = true;
        return;
    }
    case VnCommandType::HideCharacter:
        for (auto iterator = m_sceneState.stagedCharacters.begin();
             iterator != m_sceneState.stagedCharacters.end();)
        {
            if (iterator->identifier == instruction.identifier)
            {
                iterator = m_sceneState.stagedCharacters.erase(iterator);
                continue;
            }

            ++iterator;
        }
        m_sceneDirty = true;
        return;
    case VnCommandType::Name:
        m_sceneState.speakerName = instruction.text;
        m_dialogueDirty = true;
        m_sceneDirty = true;
        return;
    case VnCommandType::Text:
        m_sceneState.dialogueText = instruction.text;
        m_dialogueDirty = true;
        m_sceneDirty = true;
        m_waitingForAdvance = true;
        resetTypewriter();
        commitVisualState(false, 0.0f);
        return;
    case VnCommandType::Wait:
        commitVisualState(false, 0.0f);
        m_waitRemainingSeconds = std::max(0.0f, instruction.durationSeconds);
        return;
    case VnCommandType::Fade:
        commitVisualState(true, instruction.durationSeconds);
        return;
    case VnCommandType::Music:
        if (!instruction.text.empty())
        {
            Log::info("VNRuntime", "MUSIC placeholder: " + instruction.text);
        }
        return;
    case VnCommandType::Sfx:
        if (!instruction.text.empty())
        {
            Log::info("VNRuntime", "SFX placeholder: " + instruction.text);
        }
        return;
    case VnCommandType::End:
    {
        RuntimeTransitionRequest request{};
        request.targetId = m_returnRuntimeId;
        request.minimumDurationSeconds = 1.2f;
        request.loadingLabel = runtimeDisplayName(m_returnRuntimeId);
        requestRuntimeChange(std::move(request));
        m_finished = true;
        return;
    }
    }
}

void VNRuntime::finalizeFadeIfComplete()
{
    if (m_transitionDurationSeconds <= 0.0f ||
        m_transitionElapsedSeconds + 0.0001f < m_transitionDurationSeconds)
    {
        return;
    }

    m_activeOverlay = std::move(m_transitionTargetOverlay);
    m_transitionTargetOverlay.reset();
    m_transitionSourceOverlay.reset();
    m_transitionElapsedSeconds = 0.0f;
    m_transitionDurationSeconds = 0.0f;
}

void VNRuntime::commitVisualState(bool useFade, float fadeDurationSeconds)
{
    const bool rebuildSceneOverlay = m_sceneDirty || !m_activeOverlay.valid() || useFade;
    if (!rebuildSceneOverlay && !m_dialogueDirty)
    {
        return;
    }

    if (m_dialogueDirty)
    {
        m_dialogueOverlay = buildDialogueOverlay();
        m_dialogueDirty = false;
    }

    if (!rebuildSceneOverlay)
    {
        return;
    }

    StartupFlowOverlay nextOverlay = buildSceneOverlay();
    m_sceneDirty = false;
    if (!useFade || fadeDurationSeconds <= 0.0f)
    {
        m_activeOverlay = std::move(nextOverlay);
        m_transitionSourceOverlay.reset();
        m_transitionTargetOverlay.reset();
        m_transitionElapsedSeconds = 0.0f;
        m_transitionDurationSeconds = 0.0f;
        return;
    }

    if (m_activeOverlay.valid())
    {
        m_transitionSourceOverlay = std::move(m_activeOverlay);
    }
    else
    {
        m_transitionSourceOverlay = StartupFlowOverlay::createSolid(0, 0, 0, 255);
    }

    m_transitionTargetOverlay = std::move(nextOverlay);
    m_transitionElapsedSeconds = 0.0f;
    m_transitionDurationSeconds = std::max(0.001f, fadeDurationSeconds);
}

bool VNRuntime::updateTypewriter(float deltaSeconds, float charactersPerSecond)
{
    if (isCurrentLineFullyRevealed())
    {
        return false;
    }

    bool changed = false;
    float remainingDeltaSeconds = std::max(deltaSeconds, 0.0f);
    if (m_typewriterPauseRemainingSeconds > 0.0f)
    {
        const float consumedPauseSeconds =
            std::min(m_typewriterPauseRemainingSeconds, remainingDeltaSeconds);
        m_typewriterPauseRemainingSeconds -= consumedPauseSeconds;
        remainingDeltaSeconds -= consumedPauseSeconds;
    }

    if (m_typewriterPauseRemainingSeconds > 0.0f)
    {
        return false;
    }

    m_typewriterCarrySeconds += remainingDeltaSeconds;
    const float secondsPerCharacter = typewriterSecondsPerCharacter(charactersPerSecond);
    while (!isCurrentLineFullyRevealed() && m_typewriterCarrySeconds >= secondsPerCharacter)
    {
        m_typewriterCarrySeconds -= secondsPerCharacter;
        ++m_visibleDialogueCharacterCount;
        changed = true;

        m_typewriterPauseRemainingSeconds =
            punctuationPauseSeconds(m_visibleDialogueCharacterCount);
        if (m_typewriterPauseRemainingSeconds > 0.0f)
        {
            break;
        }
    }

    return changed;
}

void VNRuntime::resetTypewriter()
{
    m_visibleDialogueCharacterCount = 0;
    m_typewriterCarrySeconds = 0.0f;
    m_typewriterPauseRemainingSeconds = 0.0f;
    m_autoAdvanceRemainingSeconds = 0.0f;
}

void VNRuntime::completeTypewriter()
{
    m_visibleDialogueCharacterCount = m_sceneState.dialogueText.size();
    m_typewriterCarrySeconds = 0.0f;
    m_typewriterPauseRemainingSeconds = 0.0f;
    m_autoAdvanceRemainingSeconds = kAutoAdvanceDelaySeconds;
}

bool VNRuntime::isCurrentLineFullyRevealed() const
{
    return m_visibleDialogueCharacterCount >= m_sceneState.dialogueText.size();
}

std::string VNRuntime::visibleDialogueText() const
{
    return m_sceneState.dialogueText.substr(
        0, std::min(m_visibleDialogueCharacterCount, m_sceneState.dialogueText.size()));
}

float VNRuntime::punctuationPauseSeconds(const std::size_t revealedCharacterCount) const
{
    if (revealedCharacterCount == 0 || revealedCharacterCount > m_sceneState.dialogueText.size())
    {
        return 0.0f;
    }

    const char character = m_sceneState.dialogueText[revealedCharacterCount - 1];
    if (character == ',' || character == ';' || character == ':')
    {
        return 0.045f;
    }

    if (character == '.' || character == '!' || character == '?')
    {
        if (character == '.' && revealedCharacterCount >= 3 &&
            m_sceneState.dialogueText[revealedCharacterCount - 2] == '.' &&
            m_sceneState.dialogueText[revealedCharacterCount - 3] == '.')
        {
            return 0.16f;
        }

        return 0.09f;
    }

    return 0.0f;
}

StartupFlowOverlay VNRuntime::buildSceneOverlay() const
{
    if (m_assetManager == nullptr)
    {
        throw std::runtime_error("VNRuntime cannot build overlays without an AssetManager.");
    }

    VisualNovelOverlayModel overlayModel{};
    overlayModel.backgroundAssetPath = resolveTextureAssetPath(m_sceneState.backgroundAssetPath);
    overlayModel.showDialogueChrome =
        !m_sceneState.speakerName.empty() || !m_sceneState.dialogueText.empty();

    if (!overlayModel.backgroundAssetPath.empty())
    {
        const std::shared_ptr<TextureAsset> backgroundTexture =
            m_assetManager->load<TextureAsset>(overlayModel.backgroundAssetPath);
        if (backgroundTexture == nullptr)
        {
            throw std::runtime_error("VNRuntime failed to load background texture '" +
                                     overlayModel.backgroundAssetPath.string() + "'.");
        }
    }

    for (const StagedCharacter& stagedCharacter : m_sceneState.stagedCharacters)
    {
        const auto definition = m_sceneState.characterDefinitions.find(stagedCharacter.identifier);
        if (definition == m_sceneState.characterDefinitions.end())
        {
            continue;
        }

        const std::filesystem::path characterAssetPath =
            resolveTextureAssetPath(definition->second.assetPath);
        const std::shared_ptr<TextureAsset> characterTexture =
            m_assetManager->load<TextureAsset>(characterAssetPath);
        if (characterTexture == nullptr)
        {
            throw std::runtime_error("VNRuntime failed to load character texture '" +
                                     characterAssetPath.string() + "'.");
        }

        overlayModel.portraits.push_back(VisualNovelOverlayPortrait{
            characterAssetPath,
            stageAnchorXNormalized(stagedCharacter.stageRegion) + stagedCharacter.xOffsetNormalized,
            kCharacterBaselineNormalized + stagedCharacter.yOffsetNormalized,
            stagedCharacter.scale,
        });
    }

    return StartupFlowOverlay::createVisualNovelScene(*m_assetManager, overlayModel);
}

StartupFlowOverlay VNRuntime::buildDialogueOverlay() const
{
    if (m_assetManager == nullptr)
    {
        throw std::runtime_error("VNRuntime cannot build overlays without an AssetManager.");
    }

    VisualNovelOverlayModel overlayModel{};
    overlayModel.speakerName = m_sceneState.speakerName;
    overlayModel.dialogueText = visibleDialogueText();
    overlayModel.advancePrompt =
        m_waitingForAdvance && isCurrentLineFullyRevealed() && !m_autoAdvanceEnabled
            ? kAdvancePrompt
            : std::string{};

    if (overlayModel.speakerName.empty() && overlayModel.dialogueText.empty() &&
        overlayModel.advancePrompt.empty())
    {
        return {};
    }

    return StartupFlowOverlay::createVisualNovelDialogueLayer(overlayModel);
}

std::filesystem::path
VNRuntime::resolveTextureAssetPath(const std::filesystem::path& assetPath) const
{
    if (assetPath.empty())
    {
        return {};
    }

    if (assetPath.is_absolute() || assetPath.has_parent_path())
    {
        return assetPath;
    }

    return std::filesystem::path{"textures"} / assetPath;
}

float VNRuntime::stageAnchorXNormalized(VnStageRegion stageRegion) const
{
    switch (stageRegion)
    {
    case VnStageRegion::Left:
        return 0.25f;
    case VnStageRegion::Center:
        return 0.5f;
    case VnStageRegion::Right:
        return 0.75f;
    }

    return 0.5f;
}

void VNRuntime::applyOverlayTextures(Renderer& renderer) const
{
    if (m_overlayView == OverlayView::Pause)
    {
        renderer.clearSecondaryRuntimeOverlayTexture();
        const StartupFlowOverlay& pauseOverlay =
            m_pauseSelection == PauseMenuSelection::Resume
                ? m_pauseResumeOverlay
                : (m_pauseSelection == PauseMenuSelection::Settings
                       ? m_pauseSettingsOverlay
                       : (m_pauseSelection == PauseMenuSelection::ReturnToMainMenu
                              ? m_pauseReturnOverlay
                              : m_pauseIdleOverlay));
        pauseOverlay.apply(renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, 1.0f});
        return;
    }

    if (m_overlayView == OverlayView::Settings)
    {
        if (m_settingsOverlayBaseTexture.valid())
        {
            m_settingsOverlayBaseTexture.apply(
                renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, 1.0f});
            if (m_settingsOverlayContentTexture.valid())
            {
                renderer.setSecondaryRuntimeOverlayTexture(
                    m_settingsOverlayContentTexture.textureId(),
                    m_settingsOverlayContentTexture.width(),
                    m_settingsOverlayContentTexture.height(),
                    settingsContentOverlayOptions(m_settingsOverlay.viewModel()));
            }
            else
            {
                renderer.clearSecondaryRuntimeOverlayTexture();
            }
            return;
        }

        renderer.clearSecondaryRuntimeOverlayTexture();
    }

    if (m_transitionDurationSeconds > 0.0f && m_transitionSourceOverlay.valid() &&
        m_transitionTargetOverlay.valid())
    {
        const float progress = std::clamp(
            m_transitionElapsedSeconds / std::max(m_transitionDurationSeconds, 0.001f), 0.0f, 1.0f);
        m_transitionSourceOverlay.apply(
            renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, 1.0f});
        renderer.setSecondaryRuntimeOverlayTexture(
            m_transitionTargetOverlay.textureId(), m_transitionTargetOverlay.width(),
            m_transitionTargetOverlay.height(),
            RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, progress});
        return;
    }

    if (!m_activeOverlay.valid())
    {
        renderer.clearRuntimeOverlayTexture();
        renderer.clearSecondaryRuntimeOverlayTexture();
        return;
    }

    m_activeOverlay.apply(renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, 1.0f});
    if (!m_dialogueOverlay.valid())
    {
        renderer.clearSecondaryRuntimeOverlayTexture();
        return;
    }

    renderer.setSecondaryRuntimeOverlayTexture(
        m_dialogueOverlay.textureId(), m_dialogueOverlay.width(), m_dialogueOverlay.height(),
        RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, 1.0f});
}

void VNRuntime::refreshSettingsOverlay(SettingsOverlayDirtyRegion dirtyRegions)
{
    if (m_assetManager == nullptr || dirtyRegions == SettingsOverlayDirtyRegion::None)
    {
        return;
    }

    const SettingsOverlayViewModel viewModel = m_settingsOverlay.viewModel();
    if (hasDirtyRegion(dirtyRegions, SettingsOverlayDirtyRegion::Base) ||
        !m_settingsOverlayBaseTexture.valid())
    {
        m_settingsOverlayBaseTexture =
            StartupFlowOverlay::createSettingsBase(*m_assetManager, viewModel);
    }

    if (hasDirtyRegion(dirtyRegions, SettingsOverlayDirtyRegion::Content) ||
        !m_settingsOverlayContentTexture.valid())
    {
        m_settingsOverlayContentTexture = StartupFlowOverlay::createSettingsContent(viewModel);
    }
}
} // namespace engine
