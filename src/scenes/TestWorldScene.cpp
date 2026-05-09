#include "scenes/TestWorldScene.h"

#include "assets/TextureAsset.h"
#include "components/WorldComponents.h"
#include "core/Log.h"
#include "core/RenderDebug.h"
#include "core/RenderPipeline.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include "metassets/TestWorldScene.metasset.h"
#include "systems/TransformSystem.h"
#include "systems/WorldEcsSystems.h"
#include "world/Player.h"
#include "world/WorldNavigation.h"

#include <sstream>

namespace engine
{
TestWorldScene::TestWorldScene(const SceneMetasset& sceneMetasset)
    : AtmosphericSceneRuntime(sceneMetasset)
{
}

const char* TestWorldScene::name() const
{
    return "TestWorldScene";
}

void TestWorldScene::activate(AssetScope& assetScope)
{
    m_sceneAssets = std::make_unique<SceneAssetScope>(
        assetScope.assetManager, assetScope.shaderLibrary, assetScope.assetRootDirectory,
        assetScope.shaderDirectory);
    m_sceneAssets->bind(metasset());

    m_surfaceShader =
        &m_sceneAssets->requireGraphicsProgram(TestWorldSceneMetasset::kSurfaceShaderId);
    m_worldAssets =
        TestWorldAssets{&m_plane.mesh(),  &m_cube.mesh(), &m_cylinder.mesh(), &m_pyramid.mesh(),
                        &m_sphere.mesh(), &m_cone.mesh(), m_surfaceShader};

    m_scene = createAtmosphericTestWorld(m_worldSettings, m_renderSettings, m_worldAssets);

    m_runtimeOverlayTexture =
        m_sceneAssets->requireAsset<TextureAsset>(TestWorldSceneMetasset::kOverlayTextureId);

    std::ostringstream stream;
    stream << "Loaded atmospheric scene with " << m_scene.registry().entityCount()
           << " ECS entities.";
    Log::info("TestWorldScene", stream.str());
}

void TestWorldScene::deactivate(Renderer& renderer)
{
    renderer.clearRuntimeOverlayTexture();
    m_runtimeOverlayTexture.reset();
    m_surfaceShader = nullptr;
    if (m_sceneAssets != nullptr)
    {
        m_sceneAssets->clear();
        m_sceneAssets.reset();
    }
}

Scene& TestWorldScene::scene() noexcept
{
    return m_scene;
}

const Scene& TestWorldScene::scene() const noexcept
{
    return m_scene;
}

AtmosphericWorldSettings& TestWorldScene::worldSettings() noexcept
{
    return m_worldSettings;
}

const AtmosphericWorldSettings& TestWorldScene::worldSettings() const noexcept
{
    return m_worldSettings;
}

AtmosphericRenderSettings& TestWorldScene::renderSettings() noexcept
{
    return m_renderSettings;
}

const AtmosphericRenderSettings& TestWorldScene::renderSettings() const noexcept
{
    return m_renderSettings;
}

AtmosphericRuntimeState& TestWorldScene::runtimeState() noexcept
{
    return m_runtimeState;
}

const AtmosphericRuntimeState& TestWorldScene::runtimeState() const noexcept
{
    return m_runtimeState;
}

Vec3 TestWorldScene::defaultPlayerSpawn() const noexcept
{
    return Vec3{0.0f, sampleAtmosphericTerrainHeight(m_worldSettings.proceduralWorld, 0.0f, 18.0f),
                18.0f};
}

void TestWorldScene::syncWorld()
{
    syncAtmosphericTestWorld(m_scene, m_worldSettings, m_worldAssets);
}

void TestWorldScene::ensureRuntimeEntities(ecs::Entity& playerEntity,
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

void TestWorldScene::syncRuntimeEntities(ecs::Entity playerEntity, const Player& player,
                                         ecs::Entity debugCameraEntity, const Camera& debugCamera,
                                         bool debugFreeCameraEnabled)
{
    systems::syncPlayerEntity(m_scene, playerEntity, player, !debugFreeCameraEnabled);
    systems::syncCameraEntity(m_scene, debugCameraEntity, debugCamera, debugFreeCameraEnabled,
                              true);
}

void TestWorldScene::updateAtmosphere(float timeSeconds)
{
    updateAtmosphericWorldLighting(m_scene, m_worldSettings, m_renderSettings, timeSeconds);
}

void TestWorldScene::syncMoonVisual(const Vec3& activeCameraPosition)
{
    syncAtmosphericMoonVisual(m_scene, m_renderSettings, m_runtimeState, activeCameraPosition);
}

void TestWorldScene::renderWorld(Renderer& renderer, RenderPipeline& renderPipeline,
                                 int framebufferWidth, int framebufferHeight, float timeSeconds,
                                 const Camera& activeCamera, systems::FrameHistory& frameHistory,
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

const std::shared_ptr<TextureAsset>& TestWorldScene::runtimeOverlayTexture() const noexcept
{
    return m_runtimeOverlayTexture;
}
} // namespace engine
