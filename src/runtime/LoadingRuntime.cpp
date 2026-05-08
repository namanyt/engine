#include "runtime/LoadingRuntime.h"

#include "Application.h"
#include "assets/TextureAsset.h"
#include "core/Log.h"
#include "core/RenderPipeline.h"
#include "core/Renderer.h"
#include "metassets/LoadingScene.metasset.h"
#include "scenes/LoadingScene.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>

namespace engine
{
LoadingRuntime::LoadingRuntime(std::unique_ptr<RuntimeMode> nextRuntimeMode,
                               float minimumDurationSeconds, std::string nextRuntimeLabel)
    : m_nextRuntimeMode(std::move(nextRuntimeMode)),
      m_nextRuntimeLabel(std::move(nextRuntimeLabel)),
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
    setProgress(activationContext.application, 0.0f, "bootstrapping");
    m_sceneMetasset = std::make_unique<LoadingSceneMetasset>();
    m_scene = std::make_unique<LoadingScene>(*m_sceneMetasset);

    activationContext.renderer.prepareOverlayRenderingResources();
    setProgress(activationContext.application, 0.20f, "preparing loading overlay");

    SceneRuntime::AssetScope assetScope{
        activationContext.assetManager, activationContext.shaderLibrary,
        activationContext.assetRootDirectory, activationContext.shaderDirectory};
    m_scene->activate(assetScope);
    setProgress(activationContext.application, 0.40f, "activating loading scene");
    activationContext.application.setCursorCaptured(false);
    m_elapsedSeconds = 0.0f;
    applyOverlayTexture(activationContext.renderer);
    setProgress(activationContext.application, 0.55f, "binding loading resources");

    if (m_nextRuntimeMode != nullptr)
    {
        setProgress(activationContext.application, 0.65f, "preparing next runtime");
        m_nextRuntimeMode->prepareActivation(activationContext);
        setProgress(activationContext.application, 0.90f, "finalizing runtime activation");
    }
    else
    {
        setProgress(activationContext.application, 1.0f, "ready");
    }

    Log::info("LoadingRuntime", "Activated loading runtime.");
}

void LoadingRuntime::deactivate(Renderer& renderer)
{
    if (m_scene != nullptr)
    {
        m_scene->deactivate(renderer);
        m_scene.reset();
    }

    m_sceneMetasset.reset();
    m_progressPhase.clear();
    m_activationProgress = 0.0f;
    renderer.clearRuntimeOverlayTexture();
}

void LoadingRuntime::update(const UpdateContext& updateContext)
{
    (void)updateContext.inputState;
    m_elapsedSeconds += updateContext.deltaSeconds;
    updateProgressTitle(updateContext.application);
    if (m_elapsedSeconds < m_minimumDurationSeconds || m_nextRuntimeMode == nullptr)
    {
        return;
    }

    setProgress(updateContext.application, 1.0f, "complete");
    Log::info("LoadingRuntime", "Loading complete. Transitioning to " + m_nextRuntimeLabel + ".");
    requestTransition(std::move(m_nextRuntimeMode));
}

void LoadingRuntime::render(const RenderContext& renderContext)
{
    if (m_scene == nullptr)
    {
        return;
    }

    renderContext.renderer.setViewport(renderContext.framebufferWidth,
                                       renderContext.framebufferHeight);
    renderContext.renderPipeline.renderOverlayFrame(activeClearColor(renderContext.timeSeconds));
}

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
void LoadingRuntime::drawDebugUi(const DebugUiContext& debugUiContext)
{
    (void)debugUiContext;
}
#endif

Color LoadingRuntime::activeClearColor(float timeSeconds) const
{
    const Color baseColor =
        m_scene != nullptr ? m_scene->clearColor() : Color{0.02f, 0.03f, 0.05f, 1.0f};
    const float ramp = std::clamp(
        m_minimumDurationSeconds > 0.0f ? m_elapsedSeconds / m_minimumDurationSeconds : 1.0f, 0.0f,
        1.0f);
    const float pulse = 0.5f + 0.5f * std::sin(timeSeconds * 6.0f);
    return Color{baseColor.r + 0.03f * pulse, baseColor.g + 0.05f * ramp,
                 baseColor.b + 0.07f * ramp, 1.0f};
}

void LoadingRuntime::applyOverlayTexture(Renderer& renderer) const
{
    if (m_scene == nullptr || m_scene->overlayTexture() == nullptr)
    {
        renderer.clearRuntimeOverlayTexture();
        return;
    }

    renderer.setRuntimeOverlayTexture(m_scene->overlayTexture()->textureId(),
                                      m_scene->overlayTexture()->width(),
                                      m_scene->overlayTexture()->height());
}

void LoadingRuntime::setProgress(Application& application, float progress, const char* phase)
{
    m_activationProgress = std::clamp(progress, 0.0f, 1.0f);
    m_progressPhase = phase != nullptr ? phase : "loading";
    updateProgressTitle(application);
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
