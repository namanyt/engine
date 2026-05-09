#include "world/DaylightSandbox.h"

#include "components/WorldComponents.h"
#include "systems/WorldEcsSystems.h"

#include <array>
#include <cmath>
#include <string>

namespace
{
constexpr float kPi = 3.14159265359f;
constexpr float kSunVisualDistance = 520.0f;
constexpr float kSunVisualScale = 18.0f;

bool hasWorldObjectEntities(const engine::Scene& scene)
{
    bool hasWorldObjects = false;
    scene.registry().forEach<engine::components::WorldObjectComponent>(
        [&](engine::ecs::Entity, const engine::components::WorldObjectComponent&)
        { hasWorldObjects = true; });
    return hasWorldObjects;
}

engine::Vec3 computeSunDirection(const engine::AtmosphericWorldSettings& worldSettings,
                                 float timeSeconds)
{
    const float animationAngle = worldSettings.moonMotionEnabled
                                     ? worldSettings.moonTimeOffset + timeSeconds * 0.08f
                                     : worldSettings.moonTimeOffset;
    return engine::normalize(engine::Vec3{-0.46f + std::sin(animationAngle * 0.18f) * 0.05f, -1.0f,
                                          -0.12f + std::cos(animationAngle * 0.12f) * 0.04f});
}

engine::Vec3 computeSunVisualPosition(const engine::Vec3& cameraPosition,
                                      const engine::Vec3& lightDirection)
{
    return cameraPosition - engine::normalize(lightDirection) * kSunVisualDistance;
}

engine::ecs::Entity addWorldObject(engine::Scene& scene, engine::WorldObjectId id,
                                   engine::WorldObjectKind kind,
                                   engine::WorldObjectSemantic semantics, const char* debugName,
                                   const engine::Mesh* mesh, const engine::Material& material,
                                   const engine::Vec3& position, const engine::Vec3& rotation,
                                   const engine::Vec3& scale, float rayOccluderRadius = 0.0f)
{
    if (mesh == nullptr)
    {
        return engine::ecs::kInvalidEntity;
    }

    return engine::systems::spawnWorldObjectEntity(
        scene, debugName, id, kind, engine::toSemanticFlags(semantics), mesh, material, position,
        rotation, scale, true, rayOccluderRadius);
}

void addInteractable(engine::Scene& scene, const engine::ecs::Entity entity,
                     const char* interactionId, float interactionRadius)
{
    if (entity == engine::ecs::kInvalidEntity)
    {
        return;
    }

    scene.registry().emplace<engine::components::InteractableComponent>(
        entity, std::string{"Interact [E]"}, std::string{interactionId}, interactionRadius, true);
}

void addTree(engine::Scene& scene, const engine::TestWorldAssets& assets,
             const engine::Vec3& basePosition, float yaw, const engine::Material& trunkMaterial,
             const engine::Material& foliageMaterial)
{
    addWorldObject(scene, engine::WorldObjectId::None, engine::WorldObjectKind::TreeTrunk,
                   engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
                   "SandboxTreeTrunk", assets.cylinder, trunkMaterial,
                   basePosition + engine::Vec3{0.0f, 2.2f, 0.0f}, engine::Vec3{0.0f, yaw, 0.0f},
                   engine::Vec3{0.28f, 4.4f, 0.28f}, 1.05f);

    addWorldObject(scene, engine::WorldObjectId::None, engine::WorldObjectKind::TreeFoliage,
                   engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
                   "SandboxTreeFoliage", assets.cone, foliageMaterial,
                   basePosition + engine::Vec3{0.0f, 5.6f, 0.0f}, engine::Vec3{0.0f, yaw, 0.0f},
                   engine::Vec3{1.9f, 4.6f, 1.9f}, 0.88f);
    addWorldObject(scene, engine::WorldObjectId::None, engine::WorldObjectKind::TreeFoliage,
                   engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
                   "SandboxTreeFoliage", assets.cone, foliageMaterial,
                   basePosition + engine::Vec3{0.0f, 7.8f, 0.0f},
                   engine::Vec3{0.0f, yaw + 0.2f, 0.0f}, engine::Vec3{1.35f, 3.0f, 1.35f}, 0.78f);
}

void addSandboxProps(engine::Scene& scene, const engine::TestWorldAssets& assets,
                     const engine::Material& plinthMaterial, const engine::Material& objectMaterial,
                     const engine::Material& accentMaterial)
{
    const std::array<engine::Vec3, 3> plinthPositions{
        engine::Vec3{-10.0f, 1.2f, -10.0f},
        engine::Vec3{0.0f, 1.0f, -18.0f},
        engine::Vec3{11.0f, 1.4f, -8.0f},
    };

    for (std::size_t index = 0; index < plinthPositions.size(); ++index)
    {
        addWorldObject(scene, engine::WorldObjectId::None, engine::WorldObjectKind::Tower,
                       engine::WorldObjectSemantic::Surface |
                           engine::WorldObjectSemantic::RayOccluder,
                       "SandboxPlinth", assets.cylinder, plinthMaterial, plinthPositions[index],
                       engine::Vec3{},
                       engine::Vec3{1.3f, 2.0f + static_cast<float>(index) * 0.4f, 1.3f}, 0.92f);
    }

    const engine::ecs::Entity leftMarker = addWorldObject(
        scene, engine::WorldObjectId::MarkerLeft, engine::WorldObjectKind::Marker,
        engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
        "SandboxMarkerLeft", assets.cube, accentMaterial, engine::Vec3{-10.0f, 3.5f, -10.0f},
        engine::Vec3{0.0f, 0.25f, 0.0f}, engine::Vec3{1.4f, 1.4f, 1.4f}, 0.78f);
    addInteractable(scene, leftMarker, "show_alt_panel", 9.0f);
    addWorldObject(scene, engine::WorldObjectId::None, engine::WorldObjectKind::Beacon,
                   engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::Emissive,
                   "SandboxBeacon", assets.sphere, accentMaterial, engine::Vec3{0.0f, 3.8f, -18.0f},
                   engine::Vec3{}, engine::Vec3{1.0f, 1.0f, 1.0f});
    const engine::ecs::Entity rightMarker = addWorldObject(
        scene, engine::WorldObjectId::MarkerRight, engine::WorldObjectKind::Marker,
        engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
        "SandboxMarkerRight", assets.pyramid, objectMaterial, engine::Vec3{11.0f, 4.8f, -8.0f},
        engine::Vec3{0.0f, -0.2f, 0.0f}, engine::Vec3{1.8f, 2.4f, 1.8f}, 0.72f);
    addInteractable(scene, rightMarker, "restore_base_panel", 9.0f);

    addWorldObject(scene, engine::WorldObjectId::None, engine::WorldObjectKind::Rock,
                   engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
                   "SandboxStone", assets.cube, objectMaterial, engine::Vec3{-22.0f, 1.0f, -22.0f},
                   engine::Vec3{0.0f, 0.3f, 0.0f}, engine::Vec3{2.6f, 2.0f, 2.4f}, 0.85f);
    addWorldObject(scene, engine::WorldObjectId::None, engine::WorldObjectKind::Rock,
                   engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
                   "SandboxStone", assets.cube, objectMaterial, engine::Vec3{20.0f, 1.2f, -20.0f},
                   engine::Vec3{0.0f, -0.4f, 0.0f}, engine::Vec3{3.2f, 2.4f, 2.8f}, 0.85f);
}

void applySandboxDefaults(engine::AtmosphericWorldSettings& worldSettings,
                          engine::AtmosphericRenderSettings& renderSettings)
{
    renderSettings.clearColor = engine::Color{0.70f, 0.77f, 0.82f, 1.0f};
    renderSettings.fog =
        engine::FogSettings{engine::Vec3{0.78f, 0.73f, 0.66f}, 0.006f, 0.0f, 0.10f, 92.0f};
    renderSettings.sunLight = engine::DirectionalLight{
        engine::normalize(engine::Vec3{-0.46f, -1.0f, -0.12f}),
        engine::Vec3{1.00f, 0.90f, 0.72f},
        1.45f,
    };
    renderSettings.shadow = engine::ShadowSettings{
        2048, 110.0f, 1.0f, 260.0f, 0.00022f, 0.006f, 0.64f, engine::Vec3{0.0f, 8.0f, -18.0f}};
    renderSettings.skyLight = engine::SkyLight{
        engine::Vec3{0.80f, 0.76f, 0.66f},
        engine::Vec3{0.48f, 0.62f, 0.78f},
        engine::Vec3{0.36f, 0.32f, 0.26f},
        0.78f,
    };
    renderSettings.postProcess.exposure = 0.96f;
    renderSettings.postProcess.bloomThreshold = 1.45f;
    renderSettings.postProcess.bloomIntensity = 0.08f;
    renderSettings.postProcess.gamma = 2.2f;
    renderSettings.postProcess.contrast = 1.01f;
    renderSettings.postProcess.vignetteStrength = 0.08f;
    renderSettings.postProcess.saturation = 1.04f;
    renderSettings.postProcess.midtoneLift = 0.03f;

    renderSettings.rayEvaluation.shadowStrength = 0.18f;
    renderSettings.rayEvaluation.scatteringStrength = 0.90f;
    renderSettings.rayEvaluation.volumetricLightIntensity = 1.10f;
    renderSettings.rayEvaluation.directionalLightAngularRadius = 0.009f;
    renderSettings.rayEvaluation.stepLength = 4.0f;
    renderSettings.rayEvaluation.maxDistance = 600.0f;
    renderSettings.rayEvaluation.maxSteps = 96;
    renderSettings.rayEvaluation.extinctionStrength = 0.55f;
    renderSettings.rayEvaluation.atmosphericAmbientFloor = 0.08f;
    renderSettings.rayEvaluation.nearFieldHaze = 0.12f;
    renderSettings.rayEvaluation.phaseAnisotropy = 0.18f;
    renderSettings.rayEvaluation.jitterStrength = 0.10f;
    renderSettings.rayEvaluation.stepDistributionExponent = 1.2f;
    renderSettings.rayEvaluation.temporalJitterScale = 0.0f;

    worldSettings.proceduralWorld = engine::ProceduralWorldSettings{};
    worldSettings.proceduralWorld.terrainDensity = 1;
    worldSettings.proceduralWorld.terrainScale = 1.0f;
    worldSettings.proceduralWorld.terrainHeight = 0.0f;
    worldSettings.proceduralWorld.treeCount = 0;
    worldSettings.moonLightEnabled = true;
    worldSettings.sphereLightsEnabled = true;
    worldSettings.coneLightsEnabled = true;
    worldSettings.moonEmissiveEnabled = true;
    worldSettings.sphereEmissiveEnabled = true;
    worldSettings.coneEmissiveEnabled = true;
    worldSettings.moonMotionEnabled = false;
    worldSettings.moonTimeOffset = 0.0f;
}

void rebuildDaylightSandboxWorld(engine::Scene& scene,
                                 engine::AtmosphericWorldSettings& worldSettings,
                                 const engine::TestWorldAssets& assets)
{
    scene.resetWorld();

    const engine::Material groundMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::MatteStone, engine::Vec3{0.67f, 0.61f, 0.48f},
        engine::Vec3{}, 0.0f, 0.96f, 0.0f, 0.10f, 0.0f, 0.92f);
    const engine::Material trunkMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::DenseAbsorptive, engine::Vec3{0.42f, 0.31f, 0.22f},
        engine::Vec3{}, 0.0f, 0.92f, 0.0f, 0.10f, 0.0f, 0.68f);
    const engine::Material foliageMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::FogReactive, engine::Vec3{0.31f, 0.42f, 0.27f},
        engine::Vec3{0.02f, 0.04f, 0.02f}, 0.06f, 0.76f, 0.0f, 0.18f, 0.08f, 1.04f);
    const engine::Material stoneMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::MatteStone, engine::Vec3{0.66f, 0.63f, 0.58f},
        engine::Vec3{}, 0.0f, 0.88f, 0.0f, 0.16f, 0.08f, 0.96f);
    const engine::Material propMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::MetallicStructure,
        engine::Vec3{0.86f, 0.66f, 0.34f}, engine::Vec3{}, 0.0f, 0.22f, 0.24f, 0.68f, 0.04f, 1.06f);
    const engine::Material accentMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::EmissiveBeacon, engine::Vec3{0.96f, 0.82f, 0.50f},
        engine::Vec3{1.00f, 0.78f, 0.46f}, 3.2f, 0.28f, 0.0f, 0.44f, 0.04f, 1.22f);
    const engine::Material sunMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::CelestialBody, engine::Vec3{1.00f, 0.94f, 0.74f},
        engine::Vec3{1.00f, 0.94f, 0.78f}, 8.0f, 0.18f, 0.0f, 0.16f, 0.10f, 1.24f);

    addWorldObject(scene, engine::WorldObjectId::Ground, engine::WorldObjectKind::Ground,
                   engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
                   "SandboxGround", assets.ground, groundMaterial, engine::Vec3{0.0f, 0.0f, -18.0f},
                   engine::Vec3{}, engine::Vec3{220.0f, 1.0f, 220.0f}, 1.0f);

    const std::array<engine::Vec3, 8> treePositions{
        engine::Vec3{-38.0f, 0.0f, 18.0f},  engine::Vec3{-28.0f, 0.0f, -10.0f},
        engine::Vec3{-44.0f, 0.0f, -46.0f}, engine::Vec3{-16.0f, 0.0f, -62.0f},
        engine::Vec3{24.0f, 0.0f, 14.0f},   engine::Vec3{40.0f, 0.0f, -8.0f},
        engine::Vec3{30.0f, 0.0f, -44.0f},  engine::Vec3{16.0f, 0.0f, -70.0f},
    };
    for (std::size_t index = 0; index < treePositions.size(); ++index)
    {
        addTree(scene, assets, treePositions[index], static_cast<float>(index) * (kPi / 7.0f),
                trunkMaterial, foliageMaterial);
    }

    addSandboxProps(scene, assets, stoneMaterial, propMaterial, accentMaterial);

    engine::systems::spawnLocalLightEntity(
        scene, "SandboxAccentLightLeft", engine::Vec3{-10.0f, 4.8f, -10.0f},
        engine::Vec3{1.00f, 0.76f, 0.48f}, 0.65f, 16.0f, engine::LocalLightGroup::Sphere);
    engine::systems::spawnLocalLightEntity(
        scene, "SandboxAccentLightCenter", engine::Vec3{0.0f, 5.4f, -18.0f},
        engine::Vec3{1.00f, 0.84f, 0.58f}, 0.54f, 14.0f, engine::LocalLightGroup::Cone);

    addWorldObject(scene, engine::WorldObjectId::Moon, engine::WorldObjectKind::Moon,
                   engine::WorldObjectSemantic::Emissive, "SandboxSun", assets.sphere, sunMaterial,
                   engine::Vec3{0.0f, 0.0f, 0.0f}, engine::Vec3{},
                   engine::Vec3{kSunVisualScale, kSunVisualScale, kSunVisualScale});

    worldSettings.proceduralWorld.regenerationRequested = false;
}
} // namespace

namespace engine
{
Scene createDaylightSandboxWorld(AtmosphericWorldSettings& worldSettings,
                                 AtmosphericRenderSettings& renderSettings,
                                 const TestWorldAssets& assets)
{
    Scene scene{};
    applySandboxDefaults(worldSettings, renderSettings);
    worldSettings.proceduralWorld.regenerationRequested = true;
    syncDaylightSandboxWorld(scene, worldSettings, assets);
    return scene;
}

void syncDaylightSandboxWorld(Scene& scene, AtmosphericWorldSettings& worldSettings,
                              const TestWorldAssets& assets)
{
    if (!worldSettings.proceduralWorld.regenerationRequested && hasWorldObjectEntities(scene))
    {
        return;
    }

    rebuildDaylightSandboxWorld(scene, worldSettings, assets);
}

void updateDaylightSandboxLighting(Scene& scene, const AtmosphericWorldSettings& worldSettings,
                                   AtmosphericRenderSettings& renderSettings, float timeSeconds)
{
    const Vec3 sunDirection = computeSunDirection(worldSettings, timeSeconds);

    scene.registry().forEach<components::WorldObjectComponent, components::MaterialComponent>(
        [&](ecs::Entity, const components::WorldObjectComponent& object,
            components::MaterialComponent& material)
        {
            if (object.kind == WorldObjectKind::Moon)
            {
                material.material.emissiveStrength = worldSettings.moonEmissiveEnabled
                                                         ? material.material.baseEmissiveStrength
                                                         : 0.0f;
            }
            else if (object.kind == WorldObjectKind::Beacon)
            {
                material.material.emissiveStrength = worldSettings.sphereEmissiveEnabled
                                                         ? material.material.baseEmissiveStrength
                                                         : 0.0f;
            }
            else if (object.kind == WorldObjectKind::Marker)
            {
                material.material.emissiveStrength = worldSettings.coneEmissiveEnabled
                                                         ? material.material.baseEmissiveStrength
                                                         : 0.0f;
            }
        });

    scene.registry().forEach<components::LocalLightComponent>(
        [&](ecs::Entity, components::LocalLightComponent& light)
        {
            switch (light.light.group)
            {
            case LocalLightGroup::Sphere:
                light.light.intensity =
                    worldSettings.sphereLightsEnabled ? light.light.baseIntensity : 0.0f;
                break;
            case LocalLightGroup::Cone:
                light.light.intensity =
                    worldSettings.coneLightsEnabled ? light.light.baseIntensity : 0.0f;
                break;
            case LocalLightGroup::Moon:
                light.light.intensity = 0.0f;
                light.light.enabled = false;
                break;
            }
        });

    renderSettings.sunLight.direction = sunDirection;
    renderSettings.sunLight.color = Vec3{1.00f, 0.90f, 0.72f};
    renderSettings.sunLight.intensity = worldSettings.moonLightEnabled ? 1.45f : 0.0f;
    renderSettings.skyLight.horizonColor = Vec3{0.80f, 0.76f, 0.66f};
    renderSettings.skyLight.zenithColor = Vec3{0.48f, 0.62f, 0.78f};
    renderSettings.skyLight.groundColor = Vec3{0.36f, 0.32f, 0.26f};
    renderSettings.skyLight.intensity = worldSettings.moonLightEnabled ? 0.78f : 0.0f;
    renderSettings.clearColor = worldSettings.moonLightEnabled ? Color{0.70f, 0.77f, 0.82f, 1.0f}
                                                               : Color{0.14f, 0.14f, 0.16f, 1.0f};
    renderSettings.fog.color =
        worldSettings.moonLightEnabled ? Vec3{0.78f, 0.73f, 0.66f} : Vec3{0.22f, 0.22f, 0.24f};
    renderSettings.rayEvaluation.shadowStrength = worldSettings.moonLightEnabled ? 0.18f : 0.0f;
}

void syncDaylightSandboxSunVisual(Scene& scene, const AtmosphericRenderSettings& renderSettings,
                                  AtmosphericRuntimeState& runtimeState, const Vec3& cameraPosition)
{
    const Vec3 derivedSunPosition =
        computeSunVisualPosition(cameraPosition, renderSettings.sunLight.direction);
    const Vec3 sunPosition = runtimeState.debugMoonVisualOverrideEnabled
                                 ? runtimeState.debugMoonVisualOverridePosition
                                 : derivedSunPosition;
    runtimeState.moonVisualPosition = sunPosition;

    if (const ecs::Entity sunEntity = systems::findWorldObjectEntity(scene, WorldObjectId::Moon);
        sunEntity != ecs::kInvalidEntity)
    {
        if (components::TransformComponent* transform =
                scene.registry().tryGet<components::TransformComponent>(sunEntity);
            transform != nullptr)
        {
            transform->position = sunPosition;
        }
    }
}
} // namespace engine
