#include "runtime/ExplorationRuntime.h"

#include "Application.h"
#include "assets/TextureAsset.h"
#include "core/Log.h"
#include "core/RenderDebug.h"
#include "metassets/TestWorldScene.metasset.h"
#include "scenes/TestWorldScene.h"

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

    const std::shared_ptr<TextureAsset>& runtimeOverlayTexture = m_scene->runtimeOverlayTexture();
    if (runtimeOverlayTexture != nullptr)
    {
        activationContext.renderer.setRuntimeOverlayTexture(runtimeOverlayTexture->textureId(),
                                                            runtimeOverlayTexture->width(),
                                                            runtimeOverlayTexture->height());
    }
    else
    {
        activationContext.renderer.clearRuntimeOverlayTexture();
    }

    std::ostringstream stream;
    stream << "Activated exploration runtime with scene metasset '" << m_sceneMetasset->name()
           << "'.";
    Log::info("ExplorationRuntime", stream.str());
}

void ExplorationRuntime::deactivate(Renderer& renderer)
{
    renderer.clearRuntimeOverlayTexture();
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

    const ExplorationInputState inputState = interpretInput(updateContext.inputState);
    handleRuntimeInput(updateContext, inputState);

    {
        const auto terrainGenerationCpu =
            updateContext.renderer.profiler().makeCpuScope("Terrain Generation");
        m_scene->syncWorld();
    }

    m_scene->ensureRuntimeEntities(m_playerEntity, m_debugCameraEntity);
    m_scene->syncRuntimeEntities(m_playerEntity, m_player, m_debugCameraEntity, m_debugCamera,
                                 m_debugFreeCameraEnabled);

    if (m_debugFreeCameraEnabled)
    {
        m_debugCameraController.update(m_debugCamera, inputState, updateContext.deltaSeconds);
    }
    else
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

    if (inputState.toggleMoonLight)
    {
        m_scene->worldSettings().moonLightEnabled = !m_scene->worldSettings().moonLightEnabled;
    }

    if (inputState.toggleSphereLights)
    {
        m_scene->worldSettings().sphereLightsEnabled =
            !m_scene->worldSettings().sphereLightsEnabled;
    }

    if (inputState.toggleConeLights)
    {
        m_scene->worldSettings().coneLightsEnabled = !m_scene->worldSettings().coneLightsEnabled;
    }

    if (inputState.toggleMoonEmissive)
    {
        m_scene->worldSettings().moonEmissiveEnabled =
            !m_scene->worldSettings().moonEmissiveEnabled;
    }

    if (inputState.toggleSphereEmissive)
    {
        m_scene->worldSettings().sphereEmissiveEnabled =
            !m_scene->worldSettings().sphereEmissiveEnabled;
    }

    if (inputState.toggleConeEmissive)
    {
        m_scene->worldSettings().coneEmissiveEnabled =
            !m_scene->worldSettings().coneEmissiveEnabled;
    }

    if (inputState.stepMoonBackward)
    {
        m_scene->worldSettings().moonTimeOffset -= 8.0f;
    }

    if (inputState.stepMoonForward)
    {
        m_scene->worldSettings().moonTimeOffset += 8.0f;
    }

    if (inputState.toggleMoonMotion)
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
        updateContext.application.setCursorCaptured(!updateContext.application.isCursorCaptured());
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

const Camera& ExplorationRuntime::activeCamera() const
{
    return m_debugFreeCameraEnabled ? m_debugCamera : m_player.camera();
}
} // namespace engine
