#include "world/TestWorld.h"

#include "math/Types.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
engine::WorldObject makeObject(engine::WorldObjectId id, engine::WorldObjectKind kind,
                               engine::WorldObjectSemantic semantics, const char* debugName,
                               const engine::Mesh* mesh, const engine::Material& material,
                               const engine::Vec3& position, const engine::Vec3& rotation,
                               const engine::Vec3& scale)
{
    engine::WorldObject object{};
    object.debugName = debugName;
    object.id = id;
    object.kind = kind;
    object.semantics = engine::toSemanticFlags(semantics);
    object.mesh = mesh;
    object.transform.position = position;
    object.transform.rotation = rotation;
    object.transform.scale = scale;
    object.material = material;
    return object;
}

engine::Vec3 orbitingMoonPosition(float timeSeconds)
{
    const float angle = timeSeconds * 0.12f;
    return engine::Vec3{std::cos(angle) * 84.0f, 34.0f + std::sin(angle * 0.65f) * 10.0f,
                        -52.0f + std::sin(angle) * 68.0f};
}

float rayOccluderRadius(const engine::Vec3& scale, float multiplier = 1.0f)
{
    return std::max(scale.x, std::max(scale.y, scale.z)) * multiplier;
}

void appendRayOccluder(engine::Scene& scene, const engine::Vec3& center, float radius)
{
    scene.rayTracingScene.bounds.push_back(engine::BoundingSphere{center, radius});
}

void addOccludingObject(engine::Scene& scene, engine::WorldObjectId id,
                        engine::WorldObjectKind kind, engine::WorldObjectSemantic semantics,
                        const char* debugName, const engine::Mesh* mesh,
                        const engine::Material& material, const engine::Vec3& position,
                        const engine::Vec3& rotation, const engine::Vec3& scale,
                        float occluderScale = 0.0f)
{
    if (mesh == nullptr)
    {
        return;
    }

    scene.addObject(
        makeObject(id, kind, semantics, debugName, mesh, material, position, rotation, scale));

    if (occluderScale > 0.0f)
    {
        appendRayOccluder(scene, position, rayOccluderRadius(scale, occluderScale));
    }
}

void addLocalPointLight(engine::Scene& scene, const engine::Vec3& position,
                        const engine::Vec3& color, float intensity, float range,
                        engine::LocalLightGroup group)
{
    scene.localLights.push_back(engine::LocalLight{
        position,
        color,
        intensity,
        range,
        intensity,
        true,
        false,
        group,
    });
}
} // namespace

namespace engine
{
Material makeWorldMaterial(const Shader* shader, MaterialCategory category, const Vec3& albedo,
                           const Vec3& emissiveColor, float emissiveStrength, float roughness,
                           float metallic, float specularStrength, float softness,
                           float atmosphericResponse)
{
    Material material{};
    material.shader = shader;
    material.category = category;
    material.albedo = albedo;
    material.emissiveColor = emissiveColor;
    material.emissiveStrength = emissiveStrength;
    material.baseEmissiveStrength = emissiveStrength;
    material.roughness = roughness;
    material.metallic = metallic;
    material.specularStrength = specularStrength;
    material.softness = softness;
    material.atmosphericResponse = atmosphericResponse;
    return material;
}

Scene createAtmosphericTestWorld(const TestWorldAssets& assets)
{
    Scene scene{};
    scene.clearColor = Color{0.006f, 0.008f, 0.013f, 1.0f};
    scene.fog = FogSettings{Vec3{0.048f, 0.056f, 0.074f}, 0.044f, 5.2f, 0.180f};
    scene.sunLight = DirectionalLight{
        normalize(Vec3{-0.36f, -1.0f, -0.20f}),
        Vec3{0.52f, 0.60f, 0.76f},
        0.92f,
    };
    scene.shadow = ShadowSettings{2048,     84.0f,  1.0f,  210.0f,
                                  0.00035f, 0.006f, 0.66f, Vec3{0.0f, 8.0f, -52.0f}};
    scene.skyLight = SkyLight{
        Vec3{0.082f, 0.098f, 0.124f},
        Vec3{0.012f, 0.018f, 0.032f},
        Vec3{0.022f, 0.020f, 0.020f},
        0.36f,
    };
    scene.postProcess = PostProcessSettings{0.88f, 1.28f, 0.13f, 2.2f, 1.04f, 0.14f, 1.02f, 0.006f};
    scene.rayEvaluation = RayEvaluationSettings{0.16f, 0.92f, 1.55f, 0.82f, 190.0f, 96,
                                                0.54f, 0.58f, 0.70f, 0.92f, 1.48f,  0.0f};
    scene.localLights.push_back(LocalLight{orbitingMoonPosition(0.0f), Vec3{0.58f, 0.66f, 0.86f},
                                           1.85f, 180.0f, 1.85f, true, true,
                                           LocalLightGroup::Moon});

    const Material groundMaterial = makeWorldMaterial(
        assets.shader, MaterialCategory::DenseAbsorptive, Vec3{0.046f, 0.048f, 0.054f}, Vec3{},
        0.0f, 0.98f, 0.0f, 0.04f, 0.0f, 0.36f);
    const Material matteStoneMaterial =
        makeWorldMaterial(assets.shader, MaterialCategory::MatteStone, Vec3{0.46f, 0.42f, 0.38f},
                          Vec3{}, 0.0f, 0.92f, 0.02f, 0.10f, 0.32f, 0.78f);
    const Material wetReflectiveMaterial =
        makeWorldMaterial(assets.shader, MaterialCategory::WetReflective, Vec3{0.15f, 0.22f, 0.28f},
                          Vec3{}, 0.0f, 0.18f, 0.10f, 0.92f, 0.04f, 1.42f);
    const Material metallicStructureMaterial = makeWorldMaterial(
        assets.shader, MaterialCategory::MetallicStructure, Vec3{0.54f, 0.62f, 0.72f}, Vec3{}, 0.0f,
        0.24f, 0.88f, 1.00f, 0.02f, 1.12f);
    const Material emissiveBeaconMaterial = makeWorldMaterial(
        assets.shader, MaterialCategory::EmissiveBeacon, Vec3{0.38f, 0.30f, 0.18f},
        Vec3{1.00f, 0.74f, 0.28f}, 8.8f, 0.12f, 0.18f, 0.58f, 0.08f, 1.88f);
    const Material absorptiveMaterial =
        makeWorldMaterial(assets.shader, MaterialCategory::DenseAbsorptive,
                          Vec3{0.07f, 0.06f, 0.07f}, Vec3{}, 0.0f, 0.96f, 0.0f, 0.05f, 0.0f, 0.30f);
    const Material fogReactiveMaterial =
        makeWorldMaterial(assets.shader, MaterialCategory::FogReactive, Vec3{0.22f, 0.34f, 0.28f},
                          Vec3{0.08f, 0.18f, 0.16f}, 1.6f, 0.48f, 0.06f, 0.52f, 0.28f, 1.95f);
    const Material moonMaterial = makeWorldMaterial(
        assets.shader, MaterialCategory::EmissiveBeacon, Vec3{0.82f, 0.88f, 1.00f},
        Vec3{0.78f, 0.84f, 1.0f}, 14.0f, 0.08f, 0.0f, 0.10f, 0.18f, 2.10f);

    addOccludingObject(scene, WorldObjectId::Ground, WorldObjectKind::Ground,
                       WorldObjectSemantic::Surface, "Ground", assets.ground, groundMaterial,
                       Vec3{0.0f, -0.02f, -46.0f}, Vec3{0.0f, 0.0f, 0.0f},
                       Vec3{120.0f, 1.0f, 120.0f});

    const std::array<Vec3, 6> reflectivePositions{
        Vec3{-8.5f, 1.6f, -10.0f}, Vec3{8.5f, 1.6f, -10.0f},  Vec3{-8.5f, 1.6f, -22.0f},
        Vec3{8.5f, 1.6f, -22.0f},  Vec3{-8.5f, 1.6f, -34.0f}, Vec3{8.5f, 1.6f, -34.0f},
    };

    const std::array<Vec3, 6> reflectiveScales{
        Vec3{2.2f, 3.2f, 5.2f}, Vec3{2.2f, 3.2f, 5.2f}, Vec3{2.4f, 3.6f, 5.6f},
        Vec3{2.4f, 3.6f, 5.6f}, Vec3{2.6f, 4.0f, 6.2f}, Vec3{2.6f, 4.0f, 6.2f},
    };

    for (std::size_t index = 0; index < reflectivePositions.size(); ++index)
    {
        addOccludingObject(scene, WorldObjectId::None, WorldObjectKind::Tower,
                           WorldObjectSemantic::Surface | WorldObjectSemantic::RayOccluder,
                           "ReflectiveMass", assets.cube, wetReflectiveMaterial,
                           reflectivePositions[index], Vec3{0.0f, 0.0f, 0.0f},
                           reflectiveScales[index], 0.78f);
    }

    const std::array<Vec3, 6> mattePillarPositions{
        Vec3{-16.0f, 4.2f, -18.0f}, Vec3{16.0f, 4.2f, -18.0f},  Vec3{-17.0f, 5.2f, -34.0f},
        Vec3{17.0f, 5.2f, -34.0f},  Vec3{-18.0f, 6.4f, -52.0f}, Vec3{18.0f, 6.4f, -52.0f},
    };

    const std::array<Vec3, 6> mattePillarScales{
        Vec3{1.4f, 8.4f, 1.4f},  Vec3{1.4f, 8.4f, 1.4f},  Vec3{1.8f, 10.4f, 1.8f},
        Vec3{1.8f, 10.4f, 1.8f}, Vec3{2.2f, 12.8f, 2.2f}, Vec3{2.2f, 12.8f, 2.2f},
    };

    for (std::size_t index = 0; index < mattePillarPositions.size(); ++index)
    {
        addOccludingObject(scene, WorldObjectId::None, WorldObjectKind::Tower,
                           WorldObjectSemantic::Surface | WorldObjectSemantic::RayOccluder,
                           "MattePillar", assets.cylinder, matteStoneMaterial,
                           mattePillarPositions[index], Vec3{0.0f, 0.0f, 0.0f},
                           mattePillarScales[index], 0.82f);
    }

    addOccludingObject(scene, WorldObjectId::None, WorldObjectKind::Spire,
                       WorldObjectSemantic::Surface | WorldObjectSemantic::RayOccluder,
                       "MetalSpireCenter", assets.pyramid, metallicStructureMaterial,
                       Vec3{0.0f, 9.0f, -60.0f}, Vec3{0.0f, 0.0f, 0.0f}, Vec3{8.0f, 14.0f, 8.0f},
                       0.92f);
    addOccludingObject(scene, WorldObjectId::DistantSpire, WorldObjectKind::Spire,
                       WorldObjectSemantic::Surface | WorldObjectSemantic::RayOccluder,
                       "MetalSpireFar", assets.pyramid, metallicStructureMaterial,
                       Vec3{0.0f, 13.0f, -102.0f}, Vec3{0.0f, 0.35f, 0.0f},
                       Vec3{11.0f, 18.0f, 11.0f}, 0.94f);
    addOccludingObject(scene, WorldObjectId::None, WorldObjectKind::Tower,
                       WorldObjectSemantic::Surface | WorldObjectSemantic::RayOccluder,
                       "MetalMassLeft", assets.cube, metallicStructureMaterial,
                       Vec3{-11.0f, 4.0f, -58.0f}, Vec3{0.0f, 0.32f, 0.0f}, Vec3{3.4f, 8.0f, 3.4f},
                       0.80f);
    addOccludingObject(scene, WorldObjectId::None, WorldObjectKind::Tower,
                       WorldObjectSemantic::Surface | WorldObjectSemantic::RayOccluder,
                       "MetalMassRight", assets.cube, metallicStructureMaterial,
                       Vec3{11.0f, 4.0f, -58.0f}, Vec3{0.0f, -0.24f, 0.0f}, Vec3{3.4f, 8.0f, 3.4f},
                       0.80f);

    addOccludingObject(scene, WorldObjectId::MarkerLeft, WorldObjectKind::Marker,
                       WorldObjectSemantic::Surface | WorldObjectSemantic::Emissive |
                           WorldObjectSemantic::RayOccluder,
                       "FogReactiveMarkerLeft", assets.cone, fogReactiveMaterial,
                       Vec3{-24.0f, 2.6f, -46.0f}, Vec3{0.0f, 0.0f, 0.0f}, Vec3{2.8f, 5.2f, 2.8f},
                       0.84f);
    addOccludingObject(scene, WorldObjectId::MarkerRight, WorldObjectKind::Marker,
                       WorldObjectSemantic::Surface | WorldObjectSemantic::Emissive |
                           WorldObjectSemantic::RayOccluder,
                       "FogReactiveMarkerRight", assets.cone, fogReactiveMaterial,
                       Vec3{24.0f, 2.6f, -74.0f}, Vec3{0.0f, 0.18f, 0.0f}, Vec3{3.0f, 5.8f, 3.0f},
                       0.84f);
    addLocalPointLight(scene, Vec3{-24.0f, 5.4f, -46.0f}, Vec3{0.34f, 0.62f, 0.56f}, 1.25f, 14.0f,
                       LocalLightGroup::Cone);
    addLocalPointLight(scene, Vec3{24.0f, 5.8f, -74.0f}, Vec3{0.26f, 0.54f, 0.50f}, 1.35f, 15.0f,
                       LocalLightGroup::Cone);

    const std::array<Vec3, 3> beaconBases{
        Vec3{0.0f, 2.8f, -70.0f},
        Vec3{-10.5f, 2.4f, -82.0f},
        Vec3{10.5f, 2.4f, -82.0f},
    };
    const std::array<Vec3, 3> beaconTowerScales{
        Vec3{2.2f, 5.6f, 2.2f},
        Vec3{1.9f, 4.8f, 1.9f},
        Vec3{1.9f, 4.8f, 1.9f},
    };
    const std::array<float, 3> beaconRanges{18.0f, 16.0f, 16.0f};
    const std::array<float, 3> beaconIntensities{1.90f, 1.55f, 1.55f};

    for (std::size_t index = 0; index < beaconBases.size(); ++index)
    {
        addOccludingObject(scene, WorldObjectId::None, WorldObjectKind::Tower,
                           WorldObjectSemantic::Surface | WorldObjectSemantic::RayOccluder,
                           "BeaconPedestal", assets.cylinder, matteStoneMaterial,
                           beaconBases[index], Vec3{0.0f, 0.0f, 0.0f}, beaconTowerScales[index],
                           0.82f);

        const Vec3 beaconPosition =
            beaconBases[index] + Vec3{0.0f, beaconTowerScales[index].y + 1.8f, 0.0f};
        addOccludingObject(scene, WorldObjectId::None, WorldObjectKind::Beacon,
                           WorldObjectSemantic::Surface | WorldObjectSemantic::Emissive,
                           index == 0 ? "CentralBeacon" : "SideBeacon", assets.sphere,
                           emissiveBeaconMaterial, beaconPosition, Vec3{0.0f, 0.0f, 0.0f},
                           index == 0 ? Vec3{1.5f, 1.5f, 1.5f} : Vec3{1.2f, 1.2f, 1.2f});
        addLocalPointLight(scene, beaconPosition, Vec3{1.00f, 0.74f, 0.28f},
                           beaconIntensities[index], beaconRanges[index], LocalLightGroup::Sphere);
    }

    const std::array<Vec3, 8> silhouettePositions{
        Vec3{-16.0f, 3.2f, -88.0f}, Vec3{-8.0f, 4.0f, -96.0f},  Vec3{0.0f, 5.0f, -92.0f},
        Vec3{8.0f, 4.4f, -100.0f},  Vec3{16.0f, 5.4f, -108.0f}, Vec3{-12.0f, 6.2f, -114.0f},
        Vec3{2.0f, 7.0f, -118.0f},  Vec3{14.0f, 6.6f, -124.0f},
    };

    const std::array<Vec3, 8> silhouetteScales{
        Vec3{3.4f, 6.4f, 3.4f},  Vec3{3.0f, 8.0f, 3.0f},  Vec3{4.6f, 10.0f, 4.6f},
        Vec3{3.2f, 8.8f, 3.2f},  Vec3{4.0f, 10.8f, 4.0f}, Vec3{3.6f, 12.4f, 3.6f},
        Vec3{5.2f, 14.0f, 5.2f}, Vec3{4.2f, 13.2f, 4.2f},
    };

    for (std::size_t index = 0; index < silhouettePositions.size(); ++index)
    {
        addOccludingObject(
            scene, WorldObjectId::None, WorldObjectKind::Monolith,
            WorldObjectSemantic::Surface | WorldObjectSemantic::RayOccluder, "AbsorptiveMonolith",
            assets.cube, absorptiveMaterial, silhouettePositions[index],
            Vec3{0.0f, 0.12f * static_cast<float>(index), 0.0f}, silhouetteScales[index], 0.82f);
    }

    if (assets.sphere != nullptr)
    {
        WorldObject moon =
            makeObject(WorldObjectId::Moon, WorldObjectKind::Moon, WorldObjectSemantic::Emissive,
                       "Moon", assets.sphere, moonMaterial, orbitingMoonPosition(0.0f),
                       Vec3{0.0f, 0.0f, 0.0f}, Vec3{5.5f, 5.5f, 5.5f});
        moon.castsShadows = false;
        scene.addObject(moon);
    }

    return scene;
}

void updateAtmosphericWorldLighting(Scene& scene, float timeSeconds)
{
    const float effectiveTime =
        scene.moonMotionEnabled ? timeSeconds + scene.moonTimeOffset : scene.moonTimeOffset;
    const Vec3 moonPosition = orbitingMoonPosition(effectiveTime);
    const Vec3 lightFocus{0.0f, 6.0f, -44.0f};
    const Vec3 moonDirection = normalize(lightFocus - moonPosition);
    const bool anyLightingEnabled =
        scene.moonLightEnabled || scene.sphereLightsEnabled || scene.coneLightsEnabled;
    const bool anyEmissiveEnabled =
        scene.moonEmissiveEnabled || scene.sphereEmissiveEnabled || scene.coneEmissiveEnabled;
    if (WorldObject* moon = scene.findObject(WorldObjectId::Moon); moon != nullptr)
    {
        moon->transform.position = moonPosition;
        moon->material.emissiveStrength =
            scene.moonEmissiveEnabled ? moon->material.baseEmissiveStrength : 0.0f;
    }

    for (WorldObject& object : scene.objects())
    {
        if (object.kind == WorldObjectKind::Beacon)
        {
            object.material.emissiveStrength =
                scene.sphereEmissiveEnabled ? object.material.baseEmissiveStrength : 0.0f;
        }
        else if (object.kind == WorldObjectKind::Marker)
        {
            object.material.emissiveStrength =
                scene.coneEmissiveEnabled ? object.material.baseEmissiveStrength : 0.0f;
        }
    }

    scene.sunLight.direction = moonDirection;
    scene.sunLight.color = Vec3{0.58f, 0.66f, 0.84f};
    scene.sunLight.intensity = scene.moonLightEnabled ? 0.96f : 0.0f;
    scene.shadow.focusPoint = lightFocus;
    scene.skyLight.intensity = scene.moonLightEnabled ? 0.34f : 0.0f;
    scene.clearColor = (anyLightingEnabled || anyEmissiveEnabled)
                           ? Color{0.006f, 0.008f, 0.013f, 1.0f}
                           : Color{0.0f, 0.0f, 0.0f, 1.0f};
    scene.fog.color = anyLightingEnabled ? Vec3{0.048f, 0.056f, 0.074f} : Vec3{0.0f, 0.0f, 0.0f};
    scene.rayEvaluation.shadowStrength = anyLightingEnabled ? 0.12f : 0.0f;
    if (!scene.moonLightEnabled && !scene.sphereLightsEnabled && !scene.coneLightsEnabled)
    {
        scene.skyLight.intensity = 0.0f;
    }

    if (scene.localLights.size() > 8)
    {
        scene.localLights.resize(8);
    }

    if (!scene.localLights.empty())
    {
        scene.localLights[0].position = moonPosition;
        scene.localLights[0].color = Vec3{0.58f, 0.66f, 0.86f};
        scene.localLights[0].range = 180.0f;
        scene.localLights[0].baseIntensity = 1.85f;
        scene.localLights[0].intensity =
            scene.moonLightEnabled ? scene.localLights[0].baseIntensity : 0.0f;
        scene.localLights[0].enabled = true;
        scene.localLights[0].followMoon = true;
        scene.localLights[0].group = LocalLightGroup::Moon;

        for (std::size_t index = 1; index < scene.localLights.size(); ++index)
        {
            switch (scene.localLights[index].group)
            {
            case LocalLightGroup::Sphere:
                scene.localLights[index].intensity =
                    scene.sphereLightsEnabled ? scene.localLights[index].baseIntensity : 0.0f;
                break;
            case LocalLightGroup::Cone:
                scene.localLights[index].intensity =
                    scene.coneLightsEnabled ? scene.localLights[index].baseIntensity : 0.0f;
                break;
            case LocalLightGroup::Moon:
                scene.localLights[index].intensity =
                    scene.moonLightEnabled ? scene.localLights[index].baseIntensity : 0.0f;
                break;
            }
        }
    }
    else
    {
        scene.localLights.push_back(LocalLight{moonPosition, Vec3{0.58f, 0.66f, 0.86f},
                                               scene.moonLightEnabled ? 1.85f : 0.0f, 180.0f, 1.85f,
                                               true, true, LocalLightGroup::Moon});
    }
}

void setMoonLightEnabled(Scene& scene, bool enabled)
{
    scene.moonLightEnabled = enabled;
}

void setSphereLightsEnabled(Scene& scene, bool enabled)
{
    scene.sphereLightsEnabled = enabled;
}

void setConeLightsEnabled(Scene& scene, bool enabled)
{
    scene.coneLightsEnabled = enabled;
}

void setMoonMotionEnabled(Scene& scene, bool enabled)
{
    scene.moonMotionEnabled = enabled;
}

void setMoonEmissiveEnabled(Scene& scene, bool enabled)
{
    scene.moonEmissiveEnabled = enabled;
}

void setSphereEmissiveEnabled(Scene& scene, bool enabled)
{
    scene.sphereEmissiveEnabled = enabled;
}

void setConeEmissiveEnabled(Scene& scene, bool enabled)
{
    scene.coneEmissiveEnabled = enabled;
}

void stepMoonTime(Scene& scene, float offsetSeconds)
{
    scene.moonTimeOffset += offsetSeconds;
}

bool isMoonLightEnabled(const Scene& scene)
{
    return scene.moonLightEnabled;
}

bool areSphereLightsEnabled(const Scene& scene)
{
    return scene.sphereLightsEnabled;
}

bool areConeLightsEnabled(const Scene& scene)
{
    return scene.coneLightsEnabled;
}

bool isMoonEmissiveEnabled(const Scene& scene)
{
    return scene.moonEmissiveEnabled;
}

bool areSphereEmissiveEnabled(const Scene& scene)
{
    return scene.sphereEmissiveEnabled;
}

bool areConeEmissiveEnabled(const Scene& scene)
{
    return scene.coneEmissiveEnabled;
}

bool isMoonMotionEnabled(const Scene& scene)
{
    return scene.moonMotionEnabled;
}
} // namespace engine
