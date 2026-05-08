#include "runtime/ExplorationRuntime.h"

#include "Application.h"
#include "assets/TextureAsset.h"
#include "core/Log.h"
#include "core/RenderDebug.h"
#include "metassets/TestWorldScene.metasset.h"
#include "runtime/MenuRuntime.h"
#include "runtime/OverlayUiLayout.h"
#include "scenes/TestWorldScene.h"

#include <algorithm>
#include <sstream>

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
#include "debug/DebugUi.h"
#endif

namespace
{
void copyCameraPose(const engine::Camera& source, engine::Camera& destination)
{
    destination.setPosition(source.position());
    destination.setYawPitch(source.yawDegrees(), source.pitchDegrees());
}

void movePlayerToCameraPose(const engine::Camera& source, engine::Player& destination)
{
    destination.setPosition(source.position() - engine::Vec3{0.0f, destination.eyeHeight(), 0.0f});
    destination.setYawPitch(source.yawDegrees(), source.pitchDegrees());
    destination.setVelocity(engine::Vec3{});
    destination.setGrounded(false);
}
} // namespace

namespace engine
{
ExplorationRuntime::ExplorationRuntime() : m_debugCamera(Vec3{0.0f, 1.8f, 12.0f}) {}

ExplorationRuntime::~ExplorationRuntime() = default;

const char* ExplorationRuntime::name() const
{
    return "ExplorationRuntime";
}

void ExplorationRuntime::prepareActivation(ActivationContext& activationContext)
{
    if (m_sceneMetasset != nullptr && m_scene != nullptr)
    {
        return;
    }

    activationContext.renderer.prepareWorldRenderingResources();
    m_sceneMetasset = std::make_unique<TestWorldSceneMetasset>();
    m_scene = std::make_unique<TestWorldScene>(*m_sceneMetasset);

    SceneRuntime::AssetScope assetScope{
        activationContext.assetManager, activationContext.shaderLibrary,
        activationContext.assetRootDirectory, activationContext.shaderDirectory};
    m_scene->activate(assetScope);
}

void ExplorationRuntime::activate(ActivationContext& activationContext)
{
    prepareActivation(activationContext);

    const Vec3 playerSpawn{
        0.0f, sampleAtmosphericTerrainHeight(m_scene->worldSettings().proceduralWorld, 0.0f, 18.0f),
        18.0f};
    m_player.setPosition(playerSpawn);
    m_player.setYawPitch(-90.0f, -4.5f);
    copyCameraPose(m_player.camera(), m_debugCamera);
    m_debugCameraController.setMoveSpeed(10.5f);
    m_debugCameraController.setSprintMultiplier(2.1f);
    activationContext.application.setCursorCaptured(true);

    m_scene->ensureRuntimeEntities(m_playerEntity, m_debugCameraEntity);
    m_scene->syncRuntimeEntities(m_playerEntity, m_player, m_debugCameraEntity, m_debugCamera,
                                 m_debugFreeCameraEnabled);
    m_entryFadeOverlay = StartupFlowOverlay::createSolid(0, 0, 0, 255);
    m_pauseIdleOverlay = StartupFlowOverlay::createPauseMenu(PauseMenuSelection::None);
    m_pauseResumeOverlay = StartupFlowOverlay::createPauseMenu(PauseMenuSelection::Resume);
    m_pauseSettingsOverlay = StartupFlowOverlay::createPauseMenu(PauseMenuSelection::Settings);
    m_pauseReturnOverlay =
        StartupFlowOverlay::createPauseMenu(PauseMenuSelection::ReturnToMainMenu);
    m_overlayView = OverlayView::None;
    m_pauseSelection = PauseMenuSelection::None;
    m_entryFadeElapsedSeconds = 0.0f;
    applyRuntimeOverlay(activationContext.renderer);

    std::ostringstream stream;
    stream << "Activated exploration runtime with scene metasset '" << m_sceneMetasset->name()
           << "'.";
    Log::info("ExplorationRuntime", stream.str());
}

void ExplorationRuntime::deactivate(Renderer& renderer)
{
    renderer.clearRuntimeOverlayTexture();
    m_entryFadeOverlay.reset();
    m_pauseIdleOverlay.reset();
    m_pauseResumeOverlay.reset();
    m_pauseSettingsOverlay.reset();
    m_pauseReturnOverlay.reset();
    m_settingsOverlayTexture.reset();
    if (m_scene != nullptr)
    {
        m_scene->deactivate(renderer);
        m_scene.reset();
    }
    m_sceneMetasset.reset();
}

void ExplorationRuntime::update(const UpdateContext& updateContext)
{
    if (m_scene == nullptr)
    {
        return;
    }

    m_entryFadeElapsedSeconds += updateContext.deltaSeconds;

    const ExplorationInputState inputState = interpretInput(updateContext.inputState);
    if (m_overlayView == OverlayView::None)
    {
        handleRuntimeInput(updateContext, inputState);
    }
    else
    {
        handleOverlayInput(updateContext, inputState);
    }

    {
        const auto terrainGenerationCpu =
            updateContext.renderer.profiler().makeCpuScope("Terrain Generation");
        m_scene->syncWorld();
    }

    m_scene->ensureRuntimeEntities(m_playerEntity, m_debugCameraEntity);
    m_scene->syncRuntimeEntities(m_playerEntity, m_player, m_debugCameraEntity, m_debugCamera,
                                 m_debugFreeCameraEnabled);

    if (m_overlayView == OverlayView::None && m_debugFreeCameraEnabled)
    {
        m_debugCameraController.update(m_debugCamera, inputState, updateContext.deltaSeconds);
    }
    else if (m_overlayView == OverlayView::None)
    {
        m_playerController.update(m_player, m_scene->scene(),
                                  m_scene->worldSettings().proceduralWorld,
                                  m_scene->runtimeState().movementDebug, m_playerEntity, inputState,
                                  updateContext.deltaSeconds);
    }

    {
        const auto ecsUpdateCpu = updateContext.renderer.profiler().makeCpuScope("ECS Update");
        m_scene->syncRuntimeEntities(m_playerEntity, m_player, m_debugCameraEntity, m_debugCamera,
                                     m_debugFreeCameraEnabled);
    }

    if (m_overlayView == OverlayView::None && inputState.toggleMoonLight)
    {
        m_scene->worldSettings().moonLightEnabled = !m_scene->worldSettings().moonLightEnabled;
    }

    if (m_overlayView == OverlayView::None && inputState.toggleSphereLights)
    {
        m_scene->worldSettings().sphereLightsEnabled =
            !m_scene->worldSettings().sphereLightsEnabled;
    }

    if (m_overlayView == OverlayView::None && inputState.toggleConeLights)
    {
        m_scene->worldSettings().coneLightsEnabled = !m_scene->worldSettings().coneLightsEnabled;
    }

    if (m_overlayView == OverlayView::None && inputState.toggleMoonEmissive)
    {
        m_scene->worldSettings().moonEmissiveEnabled =
            !m_scene->worldSettings().moonEmissiveEnabled;
    }

    if (m_overlayView == OverlayView::None && inputState.toggleSphereEmissive)
    {
        m_scene->worldSettings().sphereEmissiveEnabled =
            !m_scene->worldSettings().sphereEmissiveEnabled;
    }

    if (m_overlayView == OverlayView::None && inputState.toggleConeEmissive)
    {
        m_scene->worldSettings().coneEmissiveEnabled =
            !m_scene->worldSettings().coneEmissiveEnabled;
    }

    if (m_overlayView == OverlayView::None && inputState.stepMoonBackward)
    {
        m_scene->worldSettings().moonTimeOffset -= 8.0f;
    }

    if (m_overlayView == OverlayView::None && inputState.stepMoonForward)
    {
        m_scene->worldSettings().moonTimeOffset += 8.0f;
    }

    if (m_overlayView == OverlayView::None && inputState.toggleMoonMotion)
    {
        m_scene->worldSettings().moonMotionEnabled = !m_scene->worldSettings().moonMotionEnabled;
    }

    {
        const auto atmosphereUpdateCpu =
            updateContext.renderer.profiler().makeCpuScope("Atmosphere Update");
        m_scene->updateAtmosphere(updateContext.timeSeconds);
    }

    m_scene->syncMoonVisual(activeCamera().position());
}

void ExplorationRuntime::render(const RenderContext& renderContext)
{
    if (m_scene == nullptr)
    {
        return;
    }

    m_scene->renderWorld(renderContext.renderer, renderContext.renderPipeline,
                         renderContext.framebufferWidth, renderContext.framebufferHeight,
                         renderContext.timeSeconds, activeCamera(), m_frameHistory);
    applyRuntimeOverlay(renderContext.renderer);
}

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
void ExplorationRuntime::drawDebugUi(const DebugUiContext& debugUiContext)
{
    if (m_scene == nullptr)
    {
        return;
    }

    const bool previousDebugFreeCameraEnabled = m_debugFreeCameraEnabled;
    const auto& registry = m_scene->scene().registry();
    const ExplorationRuntimeStats stats{static_cast<int>(registry.entityCount()),
                                        static_cast<int>(registry.componentTypeCount()),
                                        static_cast<int>(registry.totalComponentCount())};
    debugUiContext.debugUi.draw(m_scene->worldSettings(), m_scene->renderSettings(),
                                m_scene->runtimeState(), stats, m_player, m_playerController,
                                m_debugFreeCameraEnabled, debugUiContext.performanceStats,
                                debugUiContext.debugTextures);
    if (m_debugFreeCameraEnabled && !previousDebugFreeCameraEnabled)
    {
        copyCameraPose(m_player.camera(), m_debugCamera);
    }

    if (debugUiContext.debugUi.shouldQuit())
    {
        debugUiContext.application.requestQuit();
    }

    if (debugUiContext.debugUi.consumeResumeCameraRequest())
    {
        debugUiContext.application.setCursorCaptured(true);
    }
}
#endif

ExplorationInputState ExplorationRuntime::interpretInput(const RawInputState& inputState) const
{
    ExplorationInputState explorationInput{};
    explorationInput.moveForward = inputState.keyW.down;
    explorationInput.moveBackward = inputState.keyS.down;
    explorationInput.moveLeft = inputState.keyA.down;
    explorationInput.moveRight = inputState.keyD.down;
    explorationInput.moveUp = inputState.keySpace.down;
    explorationInput.moveDown = inputState.keyLeftControl.down || inputState.keyC.down;
    explorationInput.crouch = explorationInput.moveDown;
    explorationInput.jump = inputState.keySpace.down;
    explorationInput.sprint = inputState.keyLeftShift.down;
    explorationInput.toggleDebugFreeCamera = inputState.keyF2.pressed;
    explorationInput.toggleMoonLight = inputState.keyDigit1.pressed;
    explorationInput.toggleSphereLights = inputState.keyDigit2.pressed;
    explorationInput.toggleConeLights = inputState.keyDigit3.pressed;
    explorationInput.stepMoonBackward = inputState.keyDigit4.pressed;
    explorationInput.stepMoonForward = inputState.keyDigit5.pressed;
    explorationInput.toggleMoonMotion = inputState.keyDigit6.pressed;
    explorationInput.toggleMoonEmissive = inputState.keyDigit7.pressed;
    explorationInput.toggleSphereEmissive = inputState.keyDigit8.pressed;
    explorationInput.toggleConeEmissive = inputState.keyDigit9.pressed;
    explorationInput.toggleDebugUi = inputState.keyF1.pressed;
    explorationInput.toggleCursorCapture = inputState.keyEscape.pressed;
    explorationInput.cursorCaptured = inputState.cursorCaptured;
    explorationInput.mouseDelta = inputState.mouseDelta;
    return explorationInput;
}

void ExplorationRuntime::handleRuntimeInput(const UpdateContext& updateContext,
                                            const ExplorationInputState& inputState)
{
#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    if (inputState.toggleDebugUi && updateContext.debugUi != nullptr)
    {
        updateContext.debugUi->setEnabled(!updateContext.debugUi->isEnabled());
    }
#endif

    if (inputState.toggleCursorCapture)
    {
        m_overlayView = OverlayView::Pause;
        m_pauseSelection = PauseMenuSelection::None;
        updateContext.application.setCursorCaptured(false);
        return;
    }

    if (inputState.toggleDebugFreeCamera)
    {
        if (m_debugFreeCameraEnabled)
        {
            const Camera previousPlayerCamera = m_player.camera();
            movePlayerToCameraPose(m_debugCamera, m_player);
            copyCameraPose(previousPlayerCamera, m_debugCamera);
        }
        else
        {
            copyCameraPose(m_player.camera(), m_debugCamera);
        }

        m_debugFreeCameraEnabled = !m_debugFreeCameraEnabled;
    }
}

void ExplorationRuntime::handleOverlayInput(const UpdateContext& updateContext,
                                            const ExplorationInputState& inputState)
{
#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    if (inputState.toggleDebugUi && updateContext.debugUi != nullptr)
    {
        updateContext.debugUi->setEnabled(!updateContext.debugUi->isEnabled());
    }
#endif

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

        if (m_settingsOverlay.consumeDirty())
        {
            refreshSettingsOverlay();
        }
        return;
    }

    bool hoveredPauseEntry = false;
    m_pauseSelection = PauseMenuSelection::None;
    if (!updateContext.inputState.cursorCaptured && updateContext.inputState.windowSize.x > 1.0f &&
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

    if (inputState.toggleCursorCapture)
    {
        m_overlayView = OverlayView::None;
        updateContext.application.setCursorCaptured(true);
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
        updateContext.application.setCursorCaptured(true);
        return;
    }

    if (m_pauseSelection == PauseMenuSelection::Settings)
    {
        m_overlayView = OverlayView::Settings;
        m_pauseSelection = PauseMenuSelection::None;
        m_settingsOverlay.activate(updateContext.application, true);
        refreshSettingsOverlay();
        return;
    }

    requestTransition(std::make_unique<MenuRuntime>());
}

const Camera& ExplorationRuntime::activeCamera() const
{
    return m_debugFreeCameraEnabled ? m_debugCamera : m_player.camera();
}

void ExplorationRuntime::applyRuntimeOverlay(Renderer& renderer) const
{
    if (m_overlayView == OverlayView::Pause)
    {
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
        if (m_settingsOverlayTexture.valid())
        {
            m_settingsOverlayTexture.apply(
                renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, 1.0f});
            return;
        }
    }

    const float fadeProgress = std::clamp(m_entryFadeElapsedSeconds / 1.15f, 0.0f, 1.0f);
    if (fadeProgress < 0.999f && m_entryFadeOverlay.valid())
    {
        m_entryFadeOverlay.apply(
            renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, 1.0f - fadeProgress});
        return;
    }

    const std::shared_ptr<TextureAsset>& runtimeOverlayTexture = m_scene->runtimeOverlayTexture();
    if (runtimeOverlayTexture != nullptr)
    {
        renderer.setRuntimeOverlayTexture(runtimeOverlayTexture->textureId(),
                                          runtimeOverlayTexture->width(),
                                          runtimeOverlayTexture->height());
        return;
    }

    renderer.clearRuntimeOverlayTexture();
}

void ExplorationRuntime::refreshSettingsOverlay()
{
    m_settingsOverlayTexture.reset();
    m_settingsOverlayTexture =
        StartupFlowOverlay::createSettingsMenu(m_settingsOverlay.viewModel());
}
} // namespace engine
