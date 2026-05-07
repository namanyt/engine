#pragma once

#include "math/Types.h"
#include "world/Scene.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace engine
{
struct CapsuleCollisionShape final
{
    float radius = 0.42f;
    float height = 1.8f;
};

struct CollisionTriangle final
{
    Vec3 a{};
    Vec3 b{};
    Vec3 c{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec3 boundsMin{};
    Vec3 boundsMax{};
};

struct CollisionWorld final
{
    float cellSize = 8.0f;
    std::vector<CollisionTriangle> triangles{};
    std::unordered_map<long long, std::vector<std::size_t>> cellMap{};
};

struct SupportInfo final
{
    bool hit = false;
    bool walkable = false;
    float height = 0.0f;
    float distance = 0.0f;
    Vec3 point{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float slopeAngleDegrees = 0.0f;
};

struct MotionResult final
{
    Vec3 position{};
    Vec3 velocity{};
    Vec3 lastCollisionNormal{};
    Vec3 lastSurfaceMotion{};
    float residualMotionLength = 0.0f;
    int penetrationRecoveries = 0;
    int collisionCount = 0;
    int iterations = 0;
    bool exhaustedIterations = false;
    bool steppedUp = false;
};

float sampleAtmosphericTerrainHeight(const ProceduralWorldSettings& settings, float x, float z);
float sampleAtmosphericTerrainHeight(const Scene& scene, float x, float z);
float sampleAtmosphericTerrainSlope(const ProceduralWorldSettings& settings, float x, float z);
Vec3 sampleAtmosphericTerrainNormal(const ProceduralWorldSettings& settings, float x, float z);
Vec3 sampleAtmosphericTerrainNormal(const Scene& scene, float x, float z);

CollisionWorld buildCollisionWorld(const Scene& scene);
bool collisionWorldCanOccupy(const CollisionWorld& world, const Vec3& feetPosition,
                             const CapsuleCollisionShape& shape, float skinWidth = 0.0f);
SupportInfo queryGroundSupport(const CollisionWorld& world, const Vec3& feetPosition,
                               const CapsuleCollisionShape& shape, float maxSupportDistance,
                               float maxSlopeAngleDegrees);
MotionResult sweepAndSlideCapsule(const CollisionWorld& world, const Vec3& startFeetPosition,
                                  const Vec3& displacement, const Vec3& velocity,
                                  const CapsuleCollisionShape& shape, float skinWidth,
                                  int maxIterations);
} // namespace engine
