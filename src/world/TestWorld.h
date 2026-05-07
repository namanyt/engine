#pragma once

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

Material makeWorldMaterial(const Shader* shader, MaterialCategory category, const Vec3& albedo,
                           const Vec3& emissiveColor = Vec3{}, float emissiveStrength = 0.0f,
                           float roughness = 0.72f, float metallic = 0.0f,
                           float specularStrength = 0.28f, float softness = 0.0f,
                           float atmosphericResponse = 1.0f);

Scene createAtmosphericTestWorld(const TestWorldAssets& assets);
void updateAtmosphericWorldLighting(Scene& scene, float timeSeconds);
void setMoonLightEnabled(Scene& scene, bool enabled);
void setSphereLightsEnabled(Scene& scene, bool enabled);
void setConeLightsEnabled(Scene& scene, bool enabled);
void setMoonEmissiveEnabled(Scene& scene, bool enabled);
void setSphereEmissiveEnabled(Scene& scene, bool enabled);
void setConeEmissiveEnabled(Scene& scene, bool enabled);
void setMoonMotionEnabled(Scene& scene, bool enabled);
void stepMoonTime(Scene& scene, float offsetSeconds);
bool isMoonLightEnabled(const Scene& scene);
bool areSphereLightsEnabled(const Scene& scene);
bool areConeLightsEnabled(const Scene& scene);
bool isMoonEmissiveEnabled(const Scene& scene);
bool areSphereEmissiveEnabled(const Scene& scene);
bool areConeEmissiveEnabled(const Scene& scene);
bool isMoonMotionEnabled(const Scene& scene);
} // namespace engine
