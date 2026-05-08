#pragma once

#include "world/Lighting.h"
#include "world/Material.h"
#include "world/RayTracing.h"
#include "world/Scene.h"

namespace engine
{
class Mesh;
class Shader;
struct Material;

struct TestWorldAssets final
{
    const Mesh* ground = nullptr;
    const Mesh* cube = nullptr;
    const Mesh* cylinder = nullptr;
    const Mesh* pyramid = nullptr;
    const Mesh* sphere = nullptr;
    const Mesh* cone = nullptr;
    const Shader* shader = nullptr;
};

struct AtmosphericWorldSettings final
{
    ProceduralWorldSettings proceduralWorld{};
    bool moonLightEnabled = true;
    bool sphereLightsEnabled = true;
    bool coneLightsEnabled = true;
    bool moonEmissiveEnabled = true;
    bool sphereEmissiveEnabled = true;
    bool coneEmissiveEnabled = true;
    bool moonMotionEnabled = true;
    float moonTimeOffset = 0.0f;
};

struct AtmosphericRenderSettings final
{
    FogSettings fog{};
    DirectionalLight sunLight{};
    ShadowSettings shadow{};
    SkyLight skyLight{};
    PostProcessSettings postProcess{};
    RayEvaluationSettings rayEvaluation{};
    DebugViewSettings debugView{};
    Color clearColor{0.05f, 0.08f, 0.11f, 1.0f};
};

struct AtmosphericRuntimeState final
{
    MovementDebugState movementDebug{};
    Vec3 moonVisualPosition{};
    bool debugMoonVisualOverrideEnabled = false;
    Vec3 debugMoonVisualOverridePosition{};
};

Material makeWorldMaterial(const Shader* shader, MaterialCategory category, const Vec3& albedo,
                           const Vec3& emissiveColor = Vec3{}, float emissiveStrength = 0.0f,
                           float roughness = 0.72f, float metallic = 0.0f,
                           float specularStrength = 0.28f, float softness = 0.0f,
                           float atmosphericResponse = 1.0f);

Scene createAtmosphericTestWorld(AtmosphericWorldSettings& worldSettings,
                                 AtmosphericRenderSettings& renderSettings,
                                 const TestWorldAssets& assets);
void syncAtmosphericTestWorld(Scene& scene, AtmosphericWorldSettings& worldSettings,
                              const TestWorldAssets& assets);
void updateAtmosphericWorldLighting(Scene& scene, const AtmosphericWorldSettings& worldSettings,
                                    AtmosphericRenderSettings& renderSettings, float timeSeconds);
void syncAtmosphericMoonVisual(Scene& scene, const AtmosphericRenderSettings& renderSettings,
                               AtmosphericRuntimeState& runtimeState, const Vec3& cameraPosition);
} // namespace engine
