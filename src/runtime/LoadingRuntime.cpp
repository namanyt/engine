#include "runtime/LoadingRuntime.h"

#include "Application.h"
#include "core/Log.h"
#include "core/RenderPipeline.h"
#include "core/Renderer.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace engine
{
namespace
{
constexpr float kDisclaimerFadeDurationSeconds = 1.2f;
}

LoadingRuntime::LoadingRuntime(std::unique_ptr<RuntimeMode> nextRuntimeMode,
                               float minimumDurationSeconds, std::string nextRuntimeLabel,
                               LoadingScreenStyle loadingScreenStyle)
    : m_nextRuntimeMode(std::move(nextRuntimeMode)),
      m_nextRuntimeLabel(std::move(nextRuntimeLabel)), m_loadingScreenStyle(loadingScreenStyle),
      m_minimumDurationSeconds(minimumDurationSeconds)
{
    if (m_nextRuntimeLabel.empty() && m_nextRuntimeMode != nullptr)
    {
        m_nextRuntimeLabel = m_nextRuntimeMode->name();
    }
}

LoadingRuntime::~LoadingRuntime() = default;

const char* LoadingRuntime::name() const
{
    return "LoadingRuntime";
}

void LoadingRuntime::activate(ActivationContext& activationContext)
{
    m_assetManager = activationContext.assetManager;
    m_shaderLibrary = &activationContext.shaderLibrary;
    m_assetRootDirectory = activationContext.assetRootDirectory;
    m_shaderDirectory = activationContext.shaderDirectory;
    activationContext.renderer.prepareOverlayRenderingResources();
    activationContext.application.setCursorCaptured(false);
    m_elapsedSeconds = 0.0f;
    m_loadingCompletedAtSeconds = -1.0f;
    m_loadingStarted = false;
    refreshOverlay();
    setProgress(activationContext.application, 0.05f,
                m_loadingScreenStyle == LoadingScreenStyle::Disclaimer ? "entering disclaimer"
                                                                       : "starting load");

    Log::info("LoadingRuntime", "Activated loading runtime.");
}

void LoadingRuntime::deactivate(Renderer& renderer)
{
    m_loadingOverlay.reset();
    m_assetManager.reset();
    m_shaderLibrary = nullptr;
    m_assetRootDirectory.clear();
    m_shaderDirectory.clear();
    m_progressPhase.clear();
    m_activationProgress = 0.0f;
    renderer.clearRuntimeOverlayTexture();
}

void LoadingRuntime::update(const UpdateContext& updateContext)
{
    (void)updateContext.inputState;
    m_elapsedSeconds += updateContext.deltaSeconds;

    if (!m_loadingStarted && m_nextRuntimeMode != nullptr && m_elapsedSeconds >= 0.18f)
    {
        beginDeferredLoad(updateContext);
    }

    updateProgressTitle(updateContext.application);
    if (m_nextRuntimeMode == nullptr || m_loadingCompletedAtSeconds < 0.0f ||
        m_elapsedSeconds < transitionReadyTimeSeconds())
    {
        return;
    }

    setProgress(updateContext.application, 1.0f, "complete");
    Log::info("LoadingRuntime", "Loading complete. Transitioning to " + m_nextRuntimeLabel + ".");
    requestTransition(std::move(m_nextRuntimeMode));
}

void LoadingRuntime::render(const RenderContext& renderContext)
{
    applyOverlayTexture(renderContext.renderer);
    renderContext.renderer.setViewport(renderContext.framebufferWidth,
                                       renderContext.framebufferHeight);
    renderContext.renderPipeline.renderOverlayFrame(activeClearColor());
}

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
void LoadingRuntime::drawDebugUi(const DebugUiContext& debugUiContext)
{
    (void)debugUiContext;
}
#endif

Color LoadingRuntime::activeClearColor() const
{
    return Color{0.0f, 0.0f, 0.0f, 1.0f};
}

void LoadingRuntime::applyOverlayTexture(Renderer& renderer) const
{
    if (!m_loadingOverlay.valid())
    {
        renderer.clearRuntimeOverlayTexture();
        return;
    }

    m_loadingOverlay.apply(
        renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, overlayOpacity()});
}

void LoadingRuntime::beginDeferredLoad(const UpdateContext& updateContext)
{
    m_loadingStarted = true;
    setProgress(updateContext.application, 0.18f, "priming renderer");

    RuntimeMode::ActivationContext activationContext{
        updateContext.application,
        updateContext.renderer,
        updateContext.renderPipeline,
        m_assetManager,
        *m_shaderLibrary,
        m_assetRootDirectory,
        m_shaderDirectory,
    };

    setProgress(updateContext.application, 0.35f, "compiling shaders");
    m_nextRuntimeMode->prepareActivation(activationContext);
    m_loadingCompletedAtSeconds = m_elapsedSeconds;
    setProgress(updateContext.application, 0.88f, "scene ready");
}

float LoadingRuntime::overlayOpacity() const
{
    if (m_loadingScreenStyle != LoadingScreenStyle::Disclaimer)
    {
        return 1.0f;
    }

    const float fadeIn = std::clamp(m_elapsedSeconds / kDisclaimerFadeDurationSeconds, 0.0f, 1.0f);
    if (m_loadingCompletedAtSeconds < 0.0f)
    {
        return fadeIn;
    }

    const float fadeOutStart =
        std::max(m_loadingCompletedAtSeconds,
                 std::max(m_minimumDurationSeconds - kDisclaimerFadeDurationSeconds, 0.0f));
    const float fadeOut =
        1.0f -
        std::clamp((m_elapsedSeconds - fadeOutStart) / kDisclaimerFadeDurationSeconds, 0.0f, 1.0f);
    return std::clamp(std::min(fadeIn, fadeOut), 0.0f, 1.0f);
}

float LoadingRuntime::transitionReadyTimeSeconds() const
{
    if (m_loadingScreenStyle != LoadingScreenStyle::Disclaimer)
    {
        if (m_loadingCompletedAtSeconds < 0.0f)
        {
            return m_minimumDurationSeconds;
        }

        return std::max(m_loadingCompletedAtSeconds, m_minimumDurationSeconds);
    }

    if (m_loadingCompletedAtSeconds < 0.0f)
    {
        return m_minimumDurationSeconds;
    }

    return std::max(m_loadingCompletedAtSeconds,
                    std::max(m_minimumDurationSeconds - kDisclaimerFadeDurationSeconds, 0.0f)) +
           kDisclaimerFadeDurationSeconds;
}

void LoadingRuntime::setProgress(Application& application, float progress, const char* phase)
{
    m_activationProgress = std::clamp(progress, 0.0f, 1.0f);
    m_progressPhase = phase != nullptr ? phase : "loading";
    refreshOverlay();
    updateProgressTitle(application);
}

void LoadingRuntime::refreshOverlay()
{
    if (m_loadingScreenStyle == LoadingScreenStyle::Disclaimer)
    {
        m_loadingOverlay = StartupFlowOverlay::createDisclaimer();
        return;
    }

    const int percent = static_cast<int>(m_activationProgress * 100.0f + 0.5f);
    m_loadingOverlay =
        StartupFlowOverlay::createLoadingProgress(m_nextRuntimeLabel, percent, m_progressPhase);
}

void LoadingRuntime::updateProgressTitle(Application& application) const
{
    const float dwellProgress =
        m_minimumDurationSeconds > 0.0f
            ? std::clamp(m_elapsedSeconds / m_minimumDurationSeconds, 0.0f, 1.0f)
            : 1.0f;
    const float visibleProgress =
        m_activationProgress + (1.0f - m_activationProgress) * dwellProgress;
    const int percent = static_cast<int>(visibleProgress * 100.0f + 0.5f);

    std::ostringstream titleStream;
    titleStream << "EngineStarter - Loading " << m_nextRuntimeLabel << " - " << percent << "%";
    if (!m_progressPhase.empty())
    {
        titleStream << " - " << m_progressPhase;
    }

    application.setStatusWindowTitle(titleStream.str());
}
} // namespace engine
