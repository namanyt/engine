#include "runtime/MenuRuntime.h"

#include "Application.h"
#include "assets/AssetManager.h"
#include "assets/AudioAsset.h"
#include "core/AudioSystem.h"
#include "core/Log.h"
#include "core/RenderPipeline.h"
#include "core/Renderer.h"
#include "core/RuntimeOverlay.h"
#include "runtime/OverlayUiLayout.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace engine
{
namespace
{
const std::filesystem::path kMainMenuMusicPath{"audio/music.mp3"};
constexpr std::string_view kMainMenuMusicPlaybackId{"main-menu-music"};
constexpr float kMainMenuMusicVolume = 0.55f;

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

MenuRuntime::MenuRuntime() = default;

MenuRuntime::~MenuRuntime() = default;

const char* MenuRuntime::name() const
{
    return "MenuRuntime";
}

void MenuRuntime::activate(ActivationContext& activationContext)
{
    activationContext.renderer.prepareOverlayRenderingResources();

    if (activationContext.assetManager == nullptr)
    {
        throw std::runtime_error("MenuRuntime activation requires an AssetManager.");
    }

    m_assetManager = activationContext.assetManager;

    m_startSelectedOverlay =
        StartupFlowOverlay::createMenu(*m_assetManager, MainMenuSelection::NewGame);
    m_settingsSelectedOverlay =
        StartupFlowOverlay::createMenu(*m_assetManager, MainMenuSelection::Settings);
    m_quitSelectedOverlay =
        StartupFlowOverlay::createMenu(*m_assetManager, MainMenuSelection::Quit);
    m_idleOverlay = StartupFlowOverlay::createMenu(*m_assetManager, MainMenuSelection::None);
    m_phase = Phase::Browsing;
    m_view = View::Main;
    m_phaseElapsedSeconds = 0.0f;
    m_selectedAction = Selection::None;

    const std::shared_ptr<AudioAsset> menuMusic =
        m_assetManager->load<AudioAsset>(kMainMenuMusicPath);
    if (menuMusic == nullptr)
    {
        throw std::runtime_error("Failed to load main menu music asset: " +
                                 kMainMenuMusicPath.string());
    }

    activationContext.application.audioSystem().playPersistent(
        std::string(kMainMenuMusicPlaybackId), menuMusic, true, kMainMenuMusicVolume);
    activationContext.application.setCursorCaptured(false);
    Log::info("MenuRuntime", "Activated menu runtime.");
}

void MenuRuntime::deactivate(Renderer& renderer)
{
    m_startSelectedOverlay.reset();
    m_settingsSelectedOverlay.reset();
    m_quitSelectedOverlay.reset();
    m_idleOverlay.reset();
    m_settingsOverlayBaseTexture.reset();
    m_settingsOverlayContentTexture.reset();
    m_assetManager.reset();
    renderer.clearRuntimeOverlayTexture();
    renderer.clearSecondaryRuntimeOverlayTexture();
}

void MenuRuntime::update(const UpdateContext& updateContext)
{
    m_phaseElapsedSeconds += updateContext.deltaSeconds;

    const MenuInputState inputState = interpretInput(updateContext.inputState);

    if (m_view == View::Settings)
    {
        SettingsOverlay::InputState settingsInput{};
        settingsInput.cancel = inputState.cancel;
        settingsInput.click = updateContext.inputState.mouseLeft.pressed;
        settingsInput.mouseDown = updateContext.inputState.mouseLeft.down;
        settingsInput.mousePosition = updateContext.inputState.mousePosition;
        settingsInput.windowSize = updateContext.inputState.windowSize;

        if (m_settingsOverlay.update(settingsInput, updateContext.application) ==
            SettingsOverlay::Result::Close)
        {
            m_view = View::Main;
            m_selectedAction = Selection::None;
        }

        const SettingsOverlayDirtyRegion dirtyRegions = m_settingsOverlay.consumeDirtyRegions();
        if (dirtyRegions != SettingsOverlayDirtyRegion::None)
        {
            refreshSettingsOverlay(dirtyRegions);
        }
        return;
    }

    if (m_phase == Phase::FadingOut)
    {
        if (m_phaseElapsedSeconds >= 0.65f)
        {
            RuntimeTransitionRequest request{};
            request.targetId = RuntimeId::VNPrototype;
            request.minimumDurationSeconds = 7.0f;
            request.loadingLabel = "Story Prelude";
            request.loadingScreenStyle = LoadingScreenStyle::DisclaimerBootSequence;
            request.scriptAssetPath = std::filesystem::path{"scripts/test.vnscript"};
            request.returnTargetId = RuntimeId::DaylightSandbox;
            requestRuntimeChange(std::move(request));
        }

        return;
    }

    if (inputState.hoverStart)
    {
        m_selectedAction = Selection::StartExploration;
    }
    else if (inputState.hoverSettings)
    {
        m_selectedAction = Selection::Settings;
    }
    else if (inputState.hoverQuit)
    {
        m_selectedAction = Selection::Quit;
    }
    else
    {
        m_selectedAction = Selection::None;
    }

    if (inputState.cancel)
    {
        updateContext.application.requestQuit();
        return;
    }

    if (!inputState.click)
    {
        return;
    }

    if (m_selectedAction == Selection::StartExploration)
    {
        m_phase = Phase::FadingOut;
        m_phaseElapsedSeconds = 0.0f;
        return;
    }

    if (m_selectedAction == Selection::Settings)
    {
        m_view = View::Settings;
        m_selectedAction = Selection::None;
        m_settingsOverlay.activate(updateContext.application, false);
        refreshSettingsOverlay(SettingsOverlayDirtyRegion::Base |
                               SettingsOverlayDirtyRegion::Content);
        return;
    }

    updateContext.application.requestQuit();
}

void MenuRuntime::render(const RenderContext& renderContext)
{
    applyOverlayTexture(renderContext.renderer);
    renderContext.renderer.setViewport(renderContext.framebufferWidth,
                                       renderContext.framebufferHeight);
    renderContext.renderPipeline.renderOverlayFrame(activeClearColor(renderContext.timeSeconds));
}

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
void MenuRuntime::drawDebugUi(const DebugUiContext& debugUiContext)
{
    (void)debugUiContext;
}
#endif

MenuRuntime::MenuInputState MenuRuntime::interpretInput(const RawInputState& inputState) const
{
    MenuInputState menuInput{};
    menuInput.cancel = inputState.keyEscape.pressed;

    if (!inputState.cursorCaptured && inputState.windowSize.x > 1.0f &&
        inputState.windowSize.y > 1.0f)
    {
        const Vec2 designMouse =
            overlayui::toDesignSpace(inputState.mousePosition, inputState.windowSize);
        menuInput.hoverStart = overlayui::contains(overlayui::kMenuNewGameRect, designMouse);
        menuInput.hoverSettings = overlayui::contains(overlayui::kMenuSettingsRect, designMouse);
        menuInput.hoverQuit = overlayui::contains(overlayui::kMenuQuitRect, designMouse);
        menuInput.click = inputState.mouseLeft.pressed &&
                          (menuInput.hoverStart || menuInput.hoverSettings || menuInput.hoverQuit);
    }

    return menuInput;
}

Color MenuRuntime::activeClearColor(float timeSeconds) const
{
    const float sweep = 0.5f + 0.5f * std::sin(timeSeconds * 0.85f);
    const float shimmer = 0.5f + 0.5f * std::sin(timeSeconds * 1.45f + 0.8f);
    Color livelyBase{0.10f + 0.04f * sweep, 0.14f + 0.05f * shimmer, 0.18f + 0.04f * sweep, 1.0f};
    const float fade = fadeProgress();
    return Color{livelyBase.r * (1.0f - fade), livelyBase.g * (1.0f - fade),
                 livelyBase.b * (1.0f - fade), 1.0f};
}

void MenuRuntime::applyOverlayTexture(Renderer& renderer) const
{
    if (m_view == View::Settings)
    {
        if (!m_settingsOverlayBaseTexture.valid())
        {
            renderer.clearRuntimeOverlayTexture();
            renderer.clearSecondaryRuntimeOverlayTexture();
            return;
        }

        m_settingsOverlayBaseTexture.apply(
            renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, 1.0f});
        if (!m_settingsOverlayContentTexture.valid())
        {
            renderer.clearSecondaryRuntimeOverlayTexture();
            return;
        }

        renderer.setSecondaryRuntimeOverlayTexture(
            m_settingsOverlayContentTexture.textureId(), m_settingsOverlayContentTexture.width(),
            m_settingsOverlayContentTexture.height(),
            settingsContentOverlayOptions(m_settingsOverlay.viewModel()));
        return;
    }

    renderer.clearSecondaryRuntimeOverlayTexture();

    const StartupFlowOverlay& activeOverlay =
        m_selectedAction == Selection::StartExploration
            ? m_startSelectedOverlay
            : (m_selectedAction == Selection::Settings
                   ? m_settingsSelectedOverlay
                   : (m_selectedAction == Selection::Quit ? m_quitSelectedOverlay : m_idleOverlay));
    if (!activeOverlay.valid())
    {
        renderer.clearRuntimeOverlayTexture();
        return;
    }

    activeOverlay.apply(
        renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, 1.0f - fadeProgress()});
}

void MenuRuntime::refreshSettingsOverlay(SettingsOverlayDirtyRegion dirtyRegions)
{
    if (m_assetManager == nullptr)
    {
        throw std::runtime_error("MenuRuntime settings overlay requires an AssetManager.");
    }

    const SettingsOverlayViewModel viewModel = m_settingsOverlay.viewModel();
    if (hasDirtyRegion(dirtyRegions, SettingsOverlayDirtyRegion::Base))
    {
        m_settingsOverlayBaseTexture.reset();
        m_settingsOverlayBaseTexture =
            StartupFlowOverlay::createSettingsBase(*m_assetManager, viewModel);
    }

    if (hasDirtyRegion(dirtyRegions, SettingsOverlayDirtyRegion::Content))
    {
        m_settingsOverlayContentTexture.reset();
        m_settingsOverlayContentTexture = StartupFlowOverlay::createSettingsContent(viewModel);
    }
}

float MenuRuntime::fadeProgress() const
{
    if (m_phase != Phase::FadingOut)
    {
        return 0.0f;
    }

    return std::clamp(m_phaseElapsedSeconds / 0.65f, 0.0f, 1.0f);
}
} // namespace engine
