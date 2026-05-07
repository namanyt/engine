#pragma once

#include "math/Types.h"
#include "world/Scene.h"

namespace engine
{
struct CylinderCollisionShape final
{
    float radius = 0.45f;
    float height = 1.8f;
};

struct SupportHeightResult final
{
    bool hit = false;
    float height = 0.0f;
};

float sampleAtmosphericTerrainHeight(const ProceduralWorldSettings& settings, float x, float z);
float sampleAtmosphericTerrainHeight(const Scene& scene, float x, float z);
float sampleAtmosphericTerrainSlope(const ProceduralWorldSettings& settings, float x, float z);
Vec3 sampleAtmosphericTerrainNormal(const ProceduralWorldSettings& settings, float x, float z);
Vec3 sampleAtmosphericTerrainNormal(const Scene& scene, float x, float z);
SupportHeightResult sampleAtmosphericObjectSupportHeight(const Scene& scene, float x, float z,
                                                         float minimumHeight, float maximumHeight);
void resolveAtmosphericWorldCollision(const Scene& scene, const Vec3& previousFeetPosition,
                                      Vec3& feetPosition, const CylinderCollisionShape& shape,
                                      float maxStepUp);
} // namespace engine
