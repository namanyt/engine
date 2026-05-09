#include "runtime/ExplorationRuntime.h"

#include "Application.h"
#include "components/WorldComponents.h"
#include "core/Log.h"
#include "assets/TextureAsset.h"
#include "core/RenderDebug.h"
#include "metassets/DaylightSandboxScene.metasset.h"
#include "metassets/TestWorldScene.metasset.h"
#include "runtime/OverlayUiLayout.h"
#include "scenes/AtmosphericSceneRuntime.h"
#include "scenes/DaylightSandboxScene.h"
#include "systems/TransformSystem.h"
#include "scenes/TestWorldScene.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string_view>

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
#include "debug/DebugUi.h"
#endif

namespace
{
constexpr std::string_view kPromptRebuildHidden = "hidden";
constexpr std::string_view kPromptRebuildOffscreen = "offscreen";
constexpr std::string_view kPromptRebuildCached = "cached";
constexpr std::string_view kPromptRebuildOverlayMissing = "overlay_missing";
constexpr std::string_view kPromptRebuildScreenMotion = "screen_motion";
constexpr std::string_view kPromptRebuildTextChanged = "prompt_text";

std::unique_ptr<engine::SceneMetasset> createSceneMetasset(engine::RuntimeId runtimeId)
{
    switch (runtimeId)
    {
    case engine::RuntimeId::FoggyTestWorld:
        return std::make_unique<engine::TestWorldSceneMetasset>();
    case engine::RuntimeId::DaylightSandbox:
        return std::make_unique<engine::DaylightSandboxSceneMetasset>();
    case engine::RuntimeId::VNPrototype:
    case engine::RuntimeId::Menu:
        break;
    }

    throw std::runtime_error("ExplorationRuntime requires a world runtime id.");
}

std::unique_ptr<engine::AtmosphericSceneRuntime>
createSceneRuntime(engine::RuntimeId runtimeId, const engine::SceneMetasset& sceneMetasset)
{
    switch (runtimeId)
    {
    case engine::RuntimeId::FoggyTestWorld:
        return std::make_unique<engine::TestWorldScene>(sceneMetasset);
    case engine::RuntimeId::DaylightSandbox:
        return std::make_unique<engine::DaylightSandboxScene>(sceneMetasset);
    case engine::RuntimeId::VNPrototype:
    case engine::RuntimeId::Menu:
        break;
    }

    throw std::runtime_error("ExplorationRuntime cannot create a non-world scene runtime.");
}

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

std::string describeInteractionEntity(const engine::Scene& scene, const engine::ecs::Entity entity)
{
    if (entity == engine::ecs::kInvalidEntity)
    {
        return "none";
    }

    if (const engine::components::NameComponent* name =
            scene.registry().tryGet<engine::components::NameComponent>(entity);
        name != nullptr && !name->value.empty())
    {
        return name->value;
    }

    std::ostringstream stream;
    stream << "entity#" << entity;
    return stream.str();
}

float promptMeshTopOffset(const engine::components::TransformComponent& transform,
                          const engine::Mesh* mesh) noexcept
{
    if (mesh == nullptr)
    {
        return std::max(transform.scale.y * 0.6f, 0.8f);
    }

    const engine::Geometry& geometry = mesh->geometry();
    if (geometry.vertices.empty())
    {
        return std::max(transform.scale.y * 0.6f, 0.8f);
    }

    float maxLocalY = geometry.vertices.front().position.y;
    for (const engine::Vertex& vertex : geometry.vertices)
    {
        maxLocalY = std::max(maxLocalY, vertex.position.y);
    }

    return std::max(maxLocalY * std::abs(transform.scale.y), 0.6f);
}

engine::Vec3 interactionPromptAnchorForRender(const engine::Scene& scene,
                                              engine::ecs::Entity focusedEntity,
                                              const engine::Vec3& fallbackAnchor) noexcept
{
    if (focusedEntity == engine::ecs::kInvalidEntity)
    {
        return fallbackAnchor;
    }

    const auto* transform =
        scene.registry().tryGet<engine::components::TransformComponent>(focusedEntity);
    if (transform == nullptr)
    {
        return fallbackAnchor;
    }

    const auto* renderMesh =
        scene.registry().tryGet<engine::components::RenderMeshComponent>(focusedEntity);
    const float topOffset =
        promptMeshTopOffset(*transform, renderMesh != nullptr ? renderMesh->mesh : nullptr);
    return transform->position + engine::Vec3{0.0f, topOffset + 0.28f, 0.0f};
}

void appendFocusedInteractableHighlight(const engine::AtmosphericSceneRuntime& sceneRuntime,
                                        const engine::ecs::Entity focusedEntity,
                                        std::vector<engine::systems::RenderItem>& extraRenderItems)
{
    if (focusedEntity == engine::ecs::kInvalidEntity)
    {
        return;
    }

    const engine::Scene& scene = sceneRuntime.scene();
    const auto* transform =
        scene.registry().tryGet<engine::components::TransformComponent>(focusedEntity);
    const auto* interactable =
        scene.registry().tryGet<engine::components::InteractableComponent>(focusedEntity);
    const auto* renderMesh =
        scene.registry().tryGet<engine::components::RenderMeshComponent>(focusedEntity);
    const auto* materialComponent =
        scene.registry().tryGet<engine::components::MaterialComponent>(focusedEntity);
    if (transform == nullptr || interactable == nullptr || renderMesh == nullptr ||
        materialComponent == nullptr || renderMesh->mesh == nullptr ||
        materialComponent->material.shader == nullptr)
    {
        return;
    }

    engine::systems::RenderItem highlight{};
    if (const auto* name =
            scene.registry().tryGet<engine::components::NameComponent>(focusedEntity);
        name != nullptr)
    {
        highlight.debugName = name->value + ".InteractionHighlight";
    }

    if (const auto* worldObject =
            scene.registry().tryGet<engine::components::WorldObjectComponent>(focusedEntity);
        worldObject != nullptr)
    {
        highlight.id = worldObject->id;
        highlight.kind = worldObject->kind;
        highlight.semantics = worldObject->semantics;
    }

    (void)interactable;
    highlight.mesh = renderMesh->mesh;
    highlight.transform = engine::systems::TransformSystem::toLegacyTransform(*transform);
    highlight.modelMatrix = highlight.transform.modelMatrix();
    highlight.material = materialComponent->material;
    highlight.material.albedo = engine::Vec3{0.98f, 0.86f, 0.64f};
    highlight.material.emissiveColor = engine::Vec3{1.00f, 0.88f, 0.62f};
    highlight.material.emissiveStrength = std::max(highlight.material.baseEmissiveStrength, 1.5f);
    highlight.castsShadows = false;
    highlight.visible = true;
    highlight.wireframe = true;
    highlight.lineWidth = 2.2f;
    highlight.depthTest = true;
    highlight.depthWrite = false;
    highlight.doubleSided = true;
    extraRenderItems.push_back(std::move(highlight));
}
} // namespace

namespace engine
{
ExplorationRuntime::ExplorationRuntime(RuntimeId runtimeId)
    : m_runtimeId(runtimeId), m_debugCamera(Vec3{0.0f, 1.8f, 12.0f})
{
}

ExplorationRuntime::~ExplorationRuntime() = default;

const char* ExplorationRuntime::name() const
{
    return runtimeName(m_runtimeId);
}

void ExplorationRuntime::prepareActivation(ActivationContext& activationContext)
{
    if (m_sceneMetasset != nullptr && m_scene != nullptr)
    {
        return;
    }

    activationContext.renderer.prepareWorldRenderingResources();
    m_sceneMetasset = createSceneMetasset(m_runtimeId);
    m_scene = createSceneRuntime(m_runtimeId, *m_sceneMetasset);
    m_interactionPromptRenderable.prepare(activationContext.shaderLibrary);

    SceneRuntime::AssetScope assetScope{
        activationContext.assetManager, activationContext.shaderLibrary,
        activationContext.assetRootDirectory, activationContext.shaderDirectory};
    m_scene->activate(assetScope);
}

void ExplorationRuntime::activate(ActivationContext& activationContext)
{
    prepareActivation(activationContext);

    if (activationContext.assetManager == nullptr)
    {
        throw std::runtime_error("ExplorationRuntime activation requires an AssetManager.");
    }

    m_assetManager = activationContext.assetManager;

    const Vec3 playerSpawn = m_scene->defaultPlayerSpawn();
    m_player.setPosition(playerSpawn);
    m_player.setVelocity(Vec3{});
    m_player.setGrounded(false);
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
    m_alternatePanelTexture =
        m_assetManager->load<TextureAsset>(std::filesystem::path{"textures/image.png"});
    m_panelOverrideTexture.reset();
    m_interactionPromptRenderable.clear();
    m_interactionState = systems::InteractionState{};
    m_interactionPromptOpacity = 0.0f;
    m_interactionPromptWorldPosition = Vec3{};
    m_activeInteractionPrompt.clear();
    m_activeInteractionId.clear();
    m_overlayView = OverlayView::None;
    m_pauseSelection = PauseMenuSelection::None;
    m_entryFadeElapsedSeconds = 0.0f;
    applyRuntimeOverlay(activationContext.renderer);

    std::ostringstream stream;
    stream << "Activated exploration runtime '" << runtimeDisplayName(m_runtimeId)
           << "' with scene metasset '" << m_sceneMetasset->name() << "' and spawn ("
           << playerSpawn.x << ", " << playerSpawn.y << ", " << playerSpawn.z << ").";
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
    m_assetManager.reset();
    m_alternatePanelTexture.reset();
    m_panelOverrideTexture.reset();
    m_interactionPromptRenderable.clear();
    m_playerEntity = ecs::kInvalidEntity;
    m_debugCameraEntity = ecs::kInvalidEntity;
    if (m_scene != nullptr)
    {
        m_scene->deactivate(renderer);
        m_scene.reset();
    }
    m_sceneMetasset.reset();
    m_overlayView = OverlayView::None;
    m_pauseSelection = PauseMenuSelection::None;
    m_interactionState = systems::InteractionState{};
    m_interactionPromptOpacity = 0.0f;
    m_interactionPromptWorldPosition = Vec3{};
    m_activeInteractionPrompt.clear();
    m_activeInteractionId.clear();
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

    {
        const auto interactionFocusCpu =
            updateContext.renderer.profiler().makeCpuScope("Interaction Focus Update");
        updateInteractionState(inputState, updateContext.deltaSeconds);
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

    std::vector<systems::RenderItem> extraRenderItems;
    if (m_overlayView == OverlayView::None && m_scene->runtimeState().interactionDebug.enabled)
    {
        const auto interactionHighlightCpu =
            renderContext.renderer.profiler().makeCpuScope("Interaction Highlight Prep");
        appendFocusedInteractableHighlight(*m_scene, m_interactionState.focusedEntity,
                                           extraRenderItems);
    }
    appendInteractionPromptRenderItem(renderContext.renderer, extraRenderItems);
    applyRuntimeOverlay(renderContext.renderer);
    m_scene->renderWorld(renderContext.renderer, renderContext.renderPipeline,
                         renderContext.framebufferWidth, renderContext.framebufferHeight,
                         renderContext.timeSeconds, activeCamera(), m_frameHistory,
                         extraRenderItems);
}

bool ExplorationRuntime::canRenderLoadingPreview() const
{
    return m_scene != nullptr;
}

void ExplorationRuntime::renderLoadingPreview(const RenderContext& renderContext)
{
    if (m_scene == nullptr)
    {
        return;
    }

    Camera previewCamera{};
    previewCamera.setPosition(m_scene->defaultPlayerSpawn() + Vec3{0.0f, 1.8f, 0.0f});
    previewCamera.setYawPitch(-90.0f, -4.5f);

    m_scene->syncWorld();
    m_scene->updateAtmosphere(renderContext.timeSeconds);
    m_scene->syncMoonVisual(previewCamera.position());

    const std::vector<systems::RenderItem> extraRenderItems;
    m_scene->renderWorld(renderContext.renderer, renderContext.renderPipeline,
                         renderContext.framebufferWidth, renderContext.framebufferHeight,
                         renderContext.timeSeconds, previewCamera, m_loadingPreviewFrameHistory,
                         extraRenderItems);
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
                                m_debugFreeCameraEnabled, m_runtimeId,
                                debugUiContext.performanceStats, debugUiContext.debugTextures);
    if (m_debugFreeCameraEnabled != previousDebugFreeCameraEnabled)
    {
        setDebugFreeCameraEnabled(previousDebugFreeCameraEnabled, m_debugFreeCameraEnabled);
    }

    if (debugUiContext.debugUi.shouldQuit())
    {
        debugUiContext.application.requestQuit();
    }

    if (debugUiContext.debugUi.consumeResumeCameraRequest())
    {
        debugUiContext.application.setCursorCaptured(true);
    }

    if (const std::optional<RuntimeId> requestedRuntimeId =
            debugUiContext.debugUi.consumeRequestedRuntimeChange();
        requestedRuntimeId.has_value())
    {
        requestWorldLoad(*requestedRuntimeId);
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
    explorationInput.interact = inputState.keyE.pressed;
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
        setDebugFreeCameraEnabled(m_debugFreeCameraEnabled, !m_debugFreeCameraEnabled);
    }
}

void ExplorationRuntime::setDebugFreeCameraEnabled(bool previousEnabled, bool enabled)
{
    if (previousEnabled == enabled)
    {
        return;
    }

    if (enabled)
    {
        copyCameraPose(m_player.camera(), m_debugCamera);
    }
    else
    {
        movePlayerToCameraPose(m_debugCamera, m_player);
    }

    m_debugFreeCameraEnabled = enabled;
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

    RuntimeTransitionRequest request{};
    request.targetId = RuntimeId::Menu;
    request.minimumDurationSeconds = 1.1f;
    request.loadingLabel = "Main Menu";
    requestRuntimeChange(std::move(request));
}

void ExplorationRuntime::updateInteractionState(const ExplorationInputState& inputState,
                                                float deltaSeconds)
{
    if (m_scene == nullptr)
    {
        return;
    }

    const ecs::Entity previousFocusedEntity = m_interactionState.focusedEntity;
    if (m_overlayView != OverlayView::None || !inputState.cursorCaptured)
    {
        m_interactionState.focusedEntity = ecs::kInvalidEntity;
        m_activeInteractionPrompt.clear();
        m_activeInteractionId.clear();
        m_interactionPromptRenderable.clearActivePrompt();
        m_interactionPromptOpacity =
            std::max(m_interactionPromptOpacity - deltaSeconds * 3.5f, 0.0f);
        refreshInteractionDebugState();
        return;
    }

    const systems::InteractionUpdateResult interaction = systems::updateInteraction(
        m_scene->scene(), activeCamera(), inputState.interact, m_interactionState);

    m_activeInteractionPrompt = interaction.interactionPrompt;
    m_activeInteractionId = interaction.interactionId;
    m_interactionPromptRenderable.setActivePrompt(m_activeInteractionPrompt);
    m_interactionPromptWorldPosition = interaction.promptWorldPosition;
    const bool hasFocus = interaction.focusedEntity != ecs::kInvalidEntity;
    const float promptOpacityDelta = deltaSeconds * 3.5f;
    m_interactionPromptOpacity = std::clamp(
        m_interactionPromptOpacity + (hasFocus ? promptOpacityDelta : -promptOpacityDelta), 0.0f,
        1.0f);
    refreshInteractionDebugState();

    if (!interaction.interactionTriggered)
    {
        return;
    }

    const std::string entityLabel =
        describeInteractionEntity(m_scene->scene(), interaction.focusedEntity);
    if (interaction.interactionId == "show_alt_panel")
    {
        m_panelOverrideTexture = m_alternatePanelTexture;
    }
    else if (interaction.interactionId == "restore_base_panel")
    {
        m_panelOverrideTexture.reset();
    }
}

void ExplorationRuntime::appendInteractionPromptRenderItem(
    Renderer& renderer, std::vector<systems::RenderItem>& extraRenderItems)
{
    auto& interactionDebug = m_scene->runtimeState().interactionDebug;
    interactionDebug.promptVisible = false;
    interactionDebug.promptCached = false;
    interactionDebug.promptRebuiltThisFrame = false;
    interactionDebug.promptOpacity = m_interactionPromptOpacity;
    interactionDebug.promptWorldHeight = 0.0f;
    interactionDebug.promptCacheEntryCount = m_interactionPromptRenderable.cacheEntryCount();

    const auto promptRefreshCpu = renderer.profiler().makeCpuScope("Interaction Prompt Refresh");
    if (m_overlayView != OverlayView::None || m_interactionPromptOpacity <= 0.001f ||
        m_activeInteractionPrompt.empty())
    {
        interactionDebug.promptRebuildReason = std::string{kPromptRebuildHidden};
        return;
    }

    interactionDebug.promptVisible = true;

    bool rebuiltPrompt = false;
    {
        const auto billboardUpdateCpu =
            renderer.profiler().makeCpuScope("Interaction Billboard Update");
        interactionDebug.promptCached = m_interactionPromptRenderable.hasCachedActivePrompt();
        if (!interactionDebug.promptCached)
        {
            const auto promptRebuildCpu =
                renderer.profiler().makeCpuScope("Interaction Prompt Rebuild");
            rebuiltPrompt = m_interactionPromptRenderable.ensureActivePromptCached();
            interactionDebug.promptCached = m_interactionPromptRenderable.hasCachedActivePrompt();
            interactionDebug.promptCacheEntryCount =
                m_interactionPromptRenderable.cacheEntryCount();
        }

        if (!interactionDebug.promptCached)
        {
            interactionDebug.promptRebuildReason = std::string{kPromptRebuildOffscreen};
            return;
        }

        const Vec3 promptAnchor = interactionPromptAnchorForRender(
            m_scene->scene(), m_interactionState.focusedEntity, m_interactionPromptWorldPosition);
        interactionDebug.promptWorldHeight =
            m_interactionPromptRenderable.activePromptWorldHeight();
        extraRenderItems.push_back(m_interactionPromptRenderable.buildRenderItem(
            activeCamera(), promptAnchor, m_interactionPromptOpacity));
    }

    interactionDebug.promptRebuiltThisFrame = rebuiltPrompt;
    if (rebuiltPrompt)
    {
        ++interactionDebug.promptRebuildCount;
        interactionDebug.promptRebuildReason = std::string{kPromptRebuildTextChanged};
    }
    else
    {
        interactionDebug.promptRebuildReason = std::string{kPromptRebuildCached};
    }
}

const Camera& ExplorationRuntime::activeCamera() const
{
    return m_debugFreeCameraEnabled ? m_debugCamera : m_player.camera();
}

void ExplorationRuntime::applyRuntimeOverlay(Renderer& renderer) const
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
        renderer.clearSecondaryRuntimeOverlayTexture();
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
        renderer.clearSecondaryRuntimeOverlayTexture();
        m_entryFadeOverlay.apply(
            renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, 1.0f - fadeProgress});
        return;
    }

    const std::shared_ptr<TextureAsset>& runtimeOverlayTexture =
        m_panelOverrideTexture != nullptr ? m_panelOverrideTexture
                                          : m_scene->runtimeOverlayTexture();
    if (runtimeOverlayTexture != nullptr)
    {
        renderer.setRuntimeOverlayTexture(runtimeOverlayTexture->textureId(),
                                          runtimeOverlayTexture->width(),
                                          runtimeOverlayTexture->height());
    }

    renderer.clearSecondaryRuntimeOverlayTexture();
    if (runtimeOverlayTexture == nullptr)
    {
        renderer.clearRuntimeOverlayTexture();
    }
}

void ExplorationRuntime::refreshSettingsOverlay()
{
    if (m_assetManager == nullptr)
    {
        throw std::runtime_error("ExplorationRuntime settings overlay requires an AssetManager.");
    }

    m_settingsOverlayTexture.reset();
    m_settingsOverlayTexture =
        StartupFlowOverlay::createSettingsMenu(*m_assetManager, m_settingsOverlay.viewModel());
}

void ExplorationRuntime::requestWorldLoad(RuntimeId targetRuntimeId)
{
    RuntimeTransitionRequest request{};
    request.targetId = targetRuntimeId;
    request.minimumDurationSeconds = 1.6f;
    request.loadingLabel = runtimeDisplayName(targetRuntimeId);
    if (targetRuntimeId == RuntimeId::VNPrototype)
    {
        request.scriptAssetPath = std::filesystem::path{"scripts/test.vnscript"};
        request.returnTargetId = m_runtimeId;
    }

    requestRuntimeChange(std::move(request));
}

void ExplorationRuntime::refreshInteractionDebugState()
{
    if (m_scene == nullptr)
    {
        return;
    }

    auto& interactionDebug = m_scene->runtimeState().interactionDebug;
    interactionDebug.hasFocusedTarget = m_interactionState.focusedEntity != ecs::kInvalidEntity;
    interactionDebug.focusDotThreshold = systems::kInteractionFocusDotThreshold;
    interactionDebug.focusedDistance = 0.0f;
    interactionDebug.focusedAlignment = 0.0f;
    interactionDebug.promptVisible = false;
    interactionDebug.promptCached = false;
    interactionDebug.promptRebuiltThisFrame = false;
    interactionDebug.promptCacheEntryCount = m_interactionPromptRenderable.cacheEntryCount();
    interactionDebug.promptOpacity = m_interactionPromptOpacity;
    interactionDebug.promptWorldHeight = 0.0f;
    interactionDebug.focusedEntityLabel.clear();
    interactionDebug.focusedInteractionId = m_activeInteractionId;
    interactionDebug.rayOrigin = activeCamera().position();
    interactionDebug.rayEnd = activeCamera().position() + activeCamera().front() * 24.0f;

    if (interactionDebug.enabled)
    {
        interactionDebug.interactableCount = 0;
        interactionDebug.maxInteractionRadius = 0.0f;
        m_scene->scene().registry().forEach<components::InteractableComponent>(
            [&](ecs::Entity, const components::InteractableComponent& interactable)
            {
                if (!interactable.enabled)
                {
                    return;
                }

                ++interactionDebug.interactableCount;
                interactionDebug.maxInteractionRadius =
                    std::max(interactionDebug.maxInteractionRadius, interactable.interactionRadius);
            });
    }
    else
    {
        interactionDebug.interactableCount = 0;
        interactionDebug.maxInteractionRadius = 0.0f;
    }

    if (!interactionDebug.hasFocusedTarget)
    {
        return;
    }

    interactionDebug.focusedEntityLabel =
        describeInteractionEntity(m_scene->scene(), m_interactionState.focusedEntity);

    const auto* transform = m_scene->scene().registry().tryGet<components::TransformComponent>(
        m_interactionState.focusedEntity);
    if (transform == nullptr)
    {
        return;
    }

    const Vec3 toTarget = transform->position - activeCamera().position();
    const float distance = length(toTarget);
    interactionDebug.focusedDistance = distance;
    if (distance > 0.001f)
    {
        interactionDebug.focusedAlignment = dot(activeCamera().front(), toTarget / distance);
    }
}
} // namespace engine
