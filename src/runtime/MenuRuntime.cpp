#include "runtime/MenuRuntime.h"

#include "Application.h"
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
    activationContext.application.setCursorCaptured(false);
    Log::info("MenuRuntime", "Activated menu runtime.");
}

void MenuRuntime::deactivate(Renderer& renderer)
{
    m_startSelectedOverlay.reset();
    m_settingsSelectedOverlay.reset();
    m_quitSelectedOverlay.reset();
    m_idleOverlay.reset();
    m_settingsOverlayTexture.reset();
    m_assetManager.reset();
    renderer.clearRuntimeOverlayTexture();
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

        if (m_settingsOverlay.consumeDirty())
        {
            refreshSettingsOverlay();
        }
        return;
    }

    if (m_phase == Phase::FadingOut)
    {
        if (m_phaseElapsedSeconds >= 0.65f)
        {
            requestRuntimeChange(RuntimeTransitionRequest{RuntimeId::DaylightSandbox, 4.0f,
                                                          "Daylight Sandbox",
                                                          LoadingScreenStyle::Disclaimer});
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
        refreshSettingsOverlay();
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
        if (!m_settingsOverlayTexture.valid())
        {
            renderer.clearRuntimeOverlayTexture();
            return;
        }

        m_settingsOverlayTexture.apply(
            renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, 1.0f});
        return;
    }

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

void MenuRuntime::refreshSettingsOverlay()
{
    if (m_assetManager == nullptr)
    {
        throw std::runtime_error("MenuRuntime settings overlay requires an AssetManager.");
    }

    m_settingsOverlayTexture.reset();
    m_settingsOverlayTexture =
        StartupFlowOverlay::createSettingsMenu(*m_assetManager, m_settingsOverlay.viewModel());
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
