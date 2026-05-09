#include "scenes/DaylightSandboxScene.h"

#include "assets/AssetManager.h"
#include "assets/TextureAsset.h"
#include "core/RenderPipeline.h"
#include "core/Renderer.h"
#include "metassets/DaylightSandboxScene.metasset.h"
#include "metassets/SceneMetasset.h"
#include "systems/TransformSystem.h"
#include "systems/WorldEcsSystems.h"
#include "world/DaylightSandbox.h"
#include "world/Player.h"

#include <stdexcept>

namespace
{
constexpr const char* kUiOverlayTexturePath = "textures/background.png";
}

namespace engine
{
DaylightSandboxScene::DaylightSandboxScene(const SceneMetasset& sceneMetasset)
    : AtmosphericSceneRuntime(sceneMetasset)
{
}

const char* DaylightSandboxScene::name() const
{
    return "DaylightSandboxScene";
}

void DaylightSandboxScene::activate(AssetScope& assetScope)
{
    m_sceneAssets = std::make_unique<SceneAssetScope>(
        assetScope.assetManager, assetScope.shaderLibrary, assetScope.assetRootDirectory,
        assetScope.shaderDirectory);
    m_sceneAssets->bind(metasset());
    m_surfaceShader =
        &m_sceneAssets->requireGraphicsProgram(DaylightSandboxSceneMetasset::kSurfaceShaderId);

    if (assetScope.assetManager == nullptr)
    {
        throw std::runtime_error("DaylightSandboxScene activation requires an AssetManager.");
    }

    AssetManager& assetManager = *assetScope.assetManager;
    m_runtimeOverlayTexture =
        assetManager.load<TextureAsset>(std::filesystem::path{kUiOverlayTexturePath});

    m_worldAssets =
        TestWorldAssets{&m_plane.mesh(),  &m_cube.mesh(), &m_cylinder.mesh(), &m_pyramid.mesh(),
                        &m_sphere.mesh(), &m_cone.mesh(), m_surfaceShader};
    m_scene = createDaylightSandboxWorld(m_worldSettings, m_renderSettings, m_worldAssets);
}

void DaylightSandboxScene::deactivate(Renderer& renderer)
{
    renderer.clearRuntimeOverlayTexture();
    m_runtimeOverlayTexture.reset();
    m_surfaceShader = nullptr;
    if (m_sceneAssets != nullptr)
    {
        m_sceneAssets->clear();
        m_sceneAssets.reset();
    }
    m_scene = Scene{};
    m_worldAssets = TestWorldAssets{};
    m_runtimeState = AtmosphericRuntimeState{};
}

Scene& DaylightSandboxScene::scene() noexcept
{
    return m_scene;
}

const Scene& DaylightSandboxScene::scene() const noexcept
{
    return m_scene;
}

AtmosphericWorldSettings& DaylightSandboxScene::worldSettings() noexcept
{
    return m_worldSettings;
}

const AtmosphericWorldSettings& DaylightSandboxScene::worldSettings() const noexcept
{
    return m_worldSettings;
}

AtmosphericRenderSettings& DaylightSandboxScene::renderSettings() noexcept
{
    return m_renderSettings;
}

const AtmosphericRenderSettings& DaylightSandboxScene::renderSettings() const noexcept
{
    return m_renderSettings;
}

AtmosphericRuntimeState& DaylightSandboxScene::runtimeState() noexcept
{
    return m_runtimeState;
}

const AtmosphericRuntimeState& DaylightSandboxScene::runtimeState() const noexcept
{
    return m_runtimeState;
}

Vec3 DaylightSandboxScene::defaultPlayerSpawn() const noexcept
{
    return Vec3{0.0f, 0.0f, 28.0f};
}

void DaylightSandboxScene::syncWorld()
{
    syncDaylightSandboxWorld(m_scene, m_worldSettings, m_worldAssets);
}

void DaylightSandboxScene::ensureRuntimeEntities(ecs::Entity& playerEntity,
                                                 ecs::Entity& debugCameraEntity)
{
    ecs::Registry& registry = m_scene.registry();

    const bool playerEntityInvalid = !registry.isAlive(playerEntity) ||
                                     registry.has<components::WorldObjectComponent>(playerEntity) ||
                                     !registry.has<components::PlayerComponent>(playerEntity);
    if (playerEntityInvalid)
    {
        playerEntity = registry.createEntity();
    }

    const bool debugCameraEntityInvalid =
        !registry.isAlive(debugCameraEntity) ||
        registry.has<components::WorldObjectComponent>(debugCameraEntity) ||
        registry.has<components::PlayerComponent>(debugCameraEntity) ||
        !registry.has<components::CameraComponent>(debugCameraEntity);
    if (debugCameraEntityInvalid)
    {
        debugCameraEntity = registry.createEntity();
    }
}

void DaylightSandboxScene::syncRuntimeEntities(ecs::Entity playerEntity, const Player& player,
                                               ecs::Entity debugCameraEntity,
                                               const Camera& debugCamera,
                                               bool debugFreeCameraEnabled)
{
    systems::syncPlayerEntity(m_scene, playerEntity, player, !debugFreeCameraEnabled);
    systems::syncCameraEntity(m_scene, debugCameraEntity, debugCamera, debugFreeCameraEnabled,
                              true);
}

void DaylightSandboxScene::updateAtmosphere(float timeSeconds)
{
    updateDaylightSandboxLighting(m_scene, m_worldSettings, m_renderSettings, timeSeconds);
}

void DaylightSandboxScene::syncMoonVisual(const Vec3& activeCameraPosition)
{
    syncDaylightSandboxSunVisual(m_scene, m_renderSettings, m_runtimeState, activeCameraPosition);
}

void DaylightSandboxScene::renderWorld(Renderer& renderer, RenderPipeline& renderPipeline,
                                       int framebufferWidth, int framebufferHeight,
                                       float timeSeconds, const Camera& activeCamera,
                                       systems::FrameHistory& frameHistory,
                                       const std::vector<systems::RenderItem>& extraRenderItems)
{
    renderer.setViewport(framebufferWidth, framebufferHeight);
    systems::TransformSystem::updateWorldTransforms(m_scene);

    systems::RenderSceneView renderSceneView = systems::buildRenderSceneView(m_scene);
    renderSceneView.geometryItems.insert(renderSceneView.geometryItems.end(),
                                         extraRenderItems.begin(), extraRenderItems.end());
    const FrameUniforms frameUniforms =
        systems::buildFrameUniforms(m_renderSettings, activeCamera, framebufferWidth,
                                    framebufferHeight, timeSeconds, frameHistory, renderSceneView);

    renderPipeline.renderFrame(renderSceneView, m_renderSettings.clearColor,
                               m_renderSettings.postProcess, frameUniforms, timeSeconds);
    systems::advanceFrameHistory(frameUniforms, frameHistory);
}

const std::shared_ptr<TextureAsset>& DaylightSandboxScene::runtimeOverlayTexture() const noexcept
{
    return m_runtimeOverlayTexture;
}
} // namespace engine
