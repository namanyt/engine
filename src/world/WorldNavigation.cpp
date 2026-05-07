#include "world/WorldNavigation.h"

#include "components/WorldComponents.h"
#include "geometry/Geometry.h"
#include "graphics/Mesh.h"
#include "math/Transform.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace
{
constexpr float kPi = 3.14159265359f;
constexpr float kSweepEpsilon = 1.0e-4f;

float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float lerp(float start, float end, float factor)
{
    return start + (end - start) * factor;
}

float smoothstep(float start, float end, float value)
{
    if (std::abs(end - start) < 0.0001f)
    {
        return value >= end ? 1.0f : 0.0f;
    }

    const float factor = clamp01((value - start) / (end - start));
    return factor * factor * (3.0f - 2.0f * factor);
}

std::uint32_t mixBits(std::uint32_t value)
{
    value ^= value >> 16u;
    value *= 0x7feb352du;
    value ^= value >> 15u;
    value *= 0x846ca68bu;
    value ^= value >> 16u;
    return value;
}

float hash01(int x, int y, int seed)
{
    const std::uint32_t value = mixBits(static_cast<std::uint32_t>(x) * 0x1f123bb5u ^
                                        static_cast<std::uint32_t>(y) * 0x94d049bbu ^
                                        static_cast<std::uint32_t>(seed) * 0xed5ad4bbu);
    return static_cast<float>(value & 0x00ffffffu) / static_cast<float>(0x01000000u);
}

float hashSigned(int x, int y, int seed)
{
    return hash01(x, y, seed) * 2.0f - 1.0f;
}

float valueNoise(float x, float y, int seed)
{
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = x0 + 1;
    const int y1 = y0 + 1;
    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);
    const float sx = tx * tx * (3.0f - 2.0f * tx);
    const float sy = ty * ty * (3.0f - 2.0f * ty);

    const float n00 = hashSigned(x0, y0, seed);
    const float n10 = hashSigned(x1, y0, seed);
    const float n01 = hashSigned(x0, y1, seed);
    const float n11 = hashSigned(x1, y1, seed);

    const float nx0 = lerp(n00, n10, sx);
    const float nx1 = lerp(n01, n11, sx);
    return lerp(nx0, nx1, sy);
}

float fbm(float x, float y, int seed, int octaves, float persistence)
{
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float sum = 0.0f;
    float normalization = 0.0f;

    for (int octave = 0; octave < octaves; ++octave)
    {
        sum += valueNoise(x * frequency, y * frequency, seed + octave * 17) * amplitude;
        normalization += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    if (normalization <= 0.0f)
    {
        return 0.0f;
    }

    return sum / normalization;
}

float ridgeNoise(float x, float y, int seed)
{
    return 1.0f - std::abs(valueNoise(x, y, seed));
}

float radians(float degrees)
{
    return degrees * kPi / 180.0f;
}

float slopeAngleDegreesFromNormal(const engine::Vec3& normal)
{
    return std::acos(std::clamp(normal.y, -1.0f, 1.0f)) * 180.0f / kPi;
}

float walkableNormalY(float maxSlopeAngleDegrees)
{
    return std::cos(radians(maxSlopeAngleDegrees));
}

bool hasSemantic(const engine::components::WorldObjectComponent& object,
                 engine::WorldObjectSemantic semantic)
{
    return (object.semantics & engine::toSemanticFlags(semantic)) != 0u;
}

bool isMovementSolidWorldObject(const engine::components::WorldObjectComponent& object)
{
    if (!hasSemantic(object, engine::WorldObjectSemantic::Surface))
    {
        return false;
    }

    switch (object.kind)
    {
    case engine::WorldObjectKind::Ground:
    case engine::WorldObjectKind::Terrain:
    case engine::WorldObjectKind::Rock:
    case engine::WorldObjectKind::TreeTrunk:
    case engine::WorldObjectKind::Tower:
    case engine::WorldObjectKind::Beacon:
    case engine::WorldObjectKind::Monolith:
    case engine::WorldObjectKind::Spire:
    case engine::WorldObjectKind::Marker:
        return true;
    case engine::WorldObjectKind::TreeFoliage:
    case engine::WorldObjectKind::Moon:
        return false;
    }

    return false;
}

long long cellKey(int x, int z)
{
    return (static_cast<long long>(x) << 32) ^ static_cast<unsigned int>(z);
}

engine::Vec3 transformPoint(const engine::Mat4& matrix, const engine::Vec3& point)
{
    return engine::Vec3{
        matrix.elements[0] * point.x + matrix.elements[4] * point.y + matrix.elements[8] * point.z +
            matrix.elements[12],
        matrix.elements[1] * point.x + matrix.elements[5] * point.y + matrix.elements[9] * point.z +
            matrix.elements[13],
        matrix.elements[2] * point.x + matrix.elements[6] * point.y +
            matrix.elements[10] * point.z + matrix.elements[14],
    };
}

engine::Vec3 removeIntoSurfaceComponent(const engine::Vec3& value, const engine::Vec3& normal)
{
    const float inwardComponent = engine::dot(value, normal);
    if (inwardComponent >= 0.0f)
    {
        return value;
    }

    return value - normal * inwardComponent;
}

struct Bounds final
{
    engine::Vec3 minimum{};
    engine::Vec3 maximum{};
};

Bounds makeTriangleBounds(const engine::Vec3& a, const engine::Vec3& b, const engine::Vec3& c)
{
    return Bounds{engine::Vec3{std::min({a.x, b.x, c.x}), std::min({a.y, b.y, c.y}),
                               std::min({a.z, b.z, c.z})},
                  engine::Vec3{std::max({a.x, b.x, c.x}), std::max({a.y, b.y, c.y}),
                               std::max({a.z, b.z, c.z})}};
}

Bounds expandBounds(const Bounds& bounds, const engine::Vec3& expansion)
{
    return Bounds{bounds.minimum - expansion, bounds.maximum + expansion};
}

Bounds capsuleBounds(const engine::Vec3& feetPosition, const engine::CapsuleCollisionShape& shape,
                     float radius)
{
    return Bounds{engine::Vec3{feetPosition.x - radius, feetPosition.y, feetPosition.z - radius},
                  engine::Vec3{feetPosition.x + radius, feetPosition.y + shape.height,
                               feetPosition.z + radius}};
}

struct CapsuleSampleSet final
{
    std::array<float, 5> offsets{};
    int count = 0;
};

CapsuleSampleSet buildCapsuleSamples(const engine::CapsuleCollisionShape& shape, float radius)
{
    CapsuleSampleSet samples{};
    const float bottom = radius;
    const float top = std::max(shape.height - radius, bottom);
    const float span = top - bottom;
    const float sampleSpacing = std::max(radius * 0.6f, 0.16f);
    samples.count = span <= 0.001f
                        ? 1
                        : std::clamp(static_cast<int>(std::ceil(span / sampleSpacing)) + 1, 2, 5);

    for (int index = 0; index < samples.count; ++index)
    {
        const float t = samples.count == 1
                            ? 0.5f
                            : static_cast<float>(index) / static_cast<float>(samples.count - 1);
        samples.offsets[static_cast<std::size_t>(index)] = lerp(bottom, top, t);
    }

    return samples;
}

engine::Vec3 closestPointOnTriangle(const engine::Vec3& point,
                                    const engine::CollisionTriangle& triangle)
{
    const engine::Vec3 ab = triangle.b - triangle.a;
    const engine::Vec3 ac = triangle.c - triangle.a;
    const engine::Vec3 ap = point - triangle.a;
    const float d1 = engine::dot(ab, ap);
    const float d2 = engine::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
    {
        return triangle.a;
    }

    const engine::Vec3 bp = point - triangle.b;
    const float d3 = engine::dot(ab, bp);
    const float d4 = engine::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
    {
        return triangle.b;
    }

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        const float v = d1 / (d1 - d3);
        return triangle.a + ab * v;
    }

    const engine::Vec3 cp = point - triangle.c;
    const float d5 = engine::dot(ab, cp);
    const float d6 = engine::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
    {
        return triangle.c;
    }

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        const float w = d2 / (d2 - d6);
        return triangle.a + ac * w;
    }

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        const engine::Vec3 bc = triangle.c - triangle.b;
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return triangle.b + bc * w;
    }

    const float denominator = 1.0f / (va + vb + vc);
    const float v = vb * denominator;
    const float w = vc * denominator;
    return triangle.a + ab * v + ac * w;
}

std::vector<std::size_t> queryTriangleIndices(const engine::CollisionWorld& world,
                                              const Bounds& bounds)
{
    std::vector<std::size_t> matches{};
    std::vector<bool> seen(world.triangles.size(), false);
    const int minCellX = static_cast<int>(std::floor(bounds.minimum.x / world.cellSize));
    const int maxCellX = static_cast<int>(std::floor(bounds.maximum.x / world.cellSize));
    const int minCellZ = static_cast<int>(std::floor(bounds.minimum.z / world.cellSize));
    const int maxCellZ = static_cast<int>(std::floor(bounds.maximum.z / world.cellSize));

    for (int cellZ = minCellZ; cellZ <= maxCellZ; ++cellZ)
    {
        for (int cellX = minCellX; cellX <= maxCellX; ++cellX)
        {
            const auto iterator = world.cellMap.find(cellKey(cellX, cellZ));
            if (iterator == world.cellMap.end())
            {
                continue;
            }

            for (std::size_t index : iterator->second)
            {
                if (index >= seen.size() || seen[index])
                {
                    continue;
                }

                const engine::CollisionTriangle& triangle = world.triangles[index];
                if (triangle.boundsMax.y < bounds.minimum.y ||
                    triangle.boundsMin.y > bounds.maximum.y)
                {
                    continue;
                }

                seen[index] = true;
                matches.push_back(index);
            }
        }
    }

    return matches;
}

struct ContactCandidate final
{
    bool hit = false;
    float separation = std::numeric_limits<float>::infinity();
    float fraction = 1.0f;
    engine::Vec3 point{};
    engine::Vec3 normal{0.0f, 1.0f, 0.0f};
};

engine::Vec3 contactNormal(const engine::Vec3& center, const engine::Vec3& point,
                           const engine::CollisionTriangle& triangle)
{
    const engine::Vec3 delta = center - point;
    const float distance = engine::length(delta);
    if (distance > kSweepEpsilon)
    {
        return delta / distance;
    }

    engine::Vec3 normal = triangle.normal;
    if (engine::dot(center - triangle.a, normal) < 0.0f)
    {
        normal = -normal;
    }
    return normal;
}

ContactCandidate findClosestCapsuleContact(const engine::CollisionWorld& world,
                                           const engine::Vec3& feetPosition,
                                           const engine::CapsuleCollisionShape& shape, float radius,
                                           float maxSeparation)
{
    ContactCandidate best{};
    const CapsuleSampleSet samples = buildCapsuleSamples(shape, radius);
    const Bounds bounds = expandBounds(capsuleBounds(feetPosition, shape, radius),
                                       engine::Vec3{maxSeparation, maxSeparation, maxSeparation});

    for (std::size_t triangleIndex : queryTriangleIndices(world, bounds))
    {
        const engine::CollisionTriangle& triangle = world.triangles[triangleIndex];
        for (int sampleIndex = 0; sampleIndex < samples.count; ++sampleIndex)
        {
            const engine::Vec3 center =
                feetPosition +
                engine::Vec3{0.0f, samples.offsets[static_cast<std::size_t>(sampleIndex)], 0.0f};
            const engine::Vec3 point = closestPointOnTriangle(center, triangle);
            const float distance = engine::length(center - point);
            const float separation = distance - radius;
            if (separation > maxSeparation)
            {
                continue;
            }

            if (!best.hit || separation < best.separation)
            {
                best.hit = true;
                best.separation = separation;
                best.point = point;
                best.normal = contactNormal(center, point, triangle);
            }
        }
    }

    return best;
}

ContactCandidate findEarliestApproachingContact(const engine::CollisionWorld& world,
                                                const engine::Vec3& feetPosition,
                                                const engine::Vec3& displacement,
                                                const engine::CapsuleCollisionShape& shape,
                                                float radius, float skinWidth)
{
    ContactCandidate best{};
    const float motionLength = engine::length(displacement);
    if (motionLength <= kSweepEpsilon)
    {
        return best;
    }

    const Bounds currentBounds = capsuleBounds(feetPosition, shape, radius);
    const Bounds sweptBounds = expandBounds(
        Bounds{engine::Vec3{
                   std::min(currentBounds.minimum.x, currentBounds.minimum.x + displacement.x),
                   std::min(currentBounds.minimum.y, currentBounds.minimum.y + displacement.y),
                   std::min(currentBounds.minimum.z, currentBounds.minimum.z + displacement.z)},
               engine::Vec3{
                   std::max(currentBounds.maximum.x, currentBounds.maximum.x + displacement.x),
                   std::max(currentBounds.maximum.y, currentBounds.maximum.y + displacement.y),
                   std::max(currentBounds.maximum.z, currentBounds.maximum.z + displacement.z)}},
        engine::Vec3{skinWidth + radius, skinWidth + radius, skinWidth + radius});
    const CapsuleSampleSet samples = buildCapsuleSamples(shape, radius);

    for (std::size_t triangleIndex : queryTriangleIndices(world, sweptBounds))
    {
        const engine::CollisionTriangle& triangle = world.triangles[triangleIndex];
        for (int sampleIndex = 0; sampleIndex < samples.count; ++sampleIndex)
        {
            const engine::Vec3 center =
                feetPosition +
                engine::Vec3{0.0f, samples.offsets[static_cast<std::size_t>(sampleIndex)], 0.0f};
            const engine::Vec3 point = closestPointOnTriangle(center, triangle);
            const float distance = engine::length(center - point);
            const float separation = distance - radius;
            if (separation > motionLength + skinWidth)
            {
                continue;
            }

            const engine::Vec3 normal = contactNormal(center, point, triangle);
            const float approach = -engine::dot(displacement, normal);
            if (approach <= kSweepEpsilon)
            {
                continue;
            }

            const float fraction = std::clamp((separation - skinWidth) / approach, 0.0f, 1.0f);
            if (!best.hit || separation <= skinWidth + kSweepEpsilon || fraction < best.fraction)
            {
                best.hit = true;
                best.separation = separation;
                best.fraction = fraction;
                best.point = point;
                best.normal = normal;
                if (separation <= skinWidth + kSweepEpsilon)
                {
                    return best;
                }
            }
        }
    }

    return best;
}

int resolveCapsulePenetration(engine::Vec3& feetPosition, const engine::CollisionWorld& world,
                              const engine::CapsuleCollisionShape& shape, float radius,
                              float skinWidth)
{
    int recoveries = 0;
    for (int iteration = 0; iteration < 4; ++iteration)
    {
        const ContactCandidate contact =
            findClosestCapsuleContact(world, feetPosition, shape, radius, radius + skinWidth);
        if (!contact.hit || contact.separation >= -skinWidth)
        {
            break;
        }

        feetPosition = feetPosition + contact.normal * (-contact.separation + skinWidth + 0.0005f);
        ++recoveries;
    }

    return recoveries;
}
} // namespace

namespace engine
{
float sampleAtmosphericTerrainHeight(const ProceduralWorldSettings& settings, float x, float z)
{
    const float terrainScale = std::max(settings.terrainScale, 8.0f);
    const float sampleX = x / terrainScale;
    const float sampleZ = z / terrainScale;
    const float warp =
        valueNoise(sampleX * 0.34f + 11.7f, sampleZ * 0.34f - 8.4f, settings.seed + 19);
    const float warpedX = sampleX + warp * 1.35f;
    const float warpedZ = sampleZ - warp * 0.90f;

    const float broadHills = fbm(warpedX * 0.30f, warpedZ * 0.30f, settings.seed + 3, 4, 0.54f);
    const float rollingHills =
        fbm(warpedX * 0.72f - 3.0f, warpedZ * 0.72f + 5.0f, settings.seed + 11, 3, 0.58f);
    const float ridgeBands =
        ridgeNoise(warpedX * 0.42f + 7.0f, warpedZ * 0.42f - 12.0f, settings.seed + 29);
    const float farMask = smoothstep(55.0f, 190.0f, -z);
    const float sideMask = smoothstep(22.0f, 110.0f, std::abs(x));

    const float centralField =
        std::exp(-((x * x) / (44.0f * 44.0f) + ((z + 20.0f) * (z + 20.0f)) / (96.0f * 96.0f)));
    const float leftBasin = std::exp(-(((x + 34.0f) * (x + 34.0f)) / (34.0f * 34.0f) +
                                       ((z + 70.0f) * (z + 70.0f)) / (26.0f * 26.0f)));
    const float rightBasin = std::exp(-(((x - 28.0f) * (x - 28.0f)) / (28.0f * 28.0f) +
                                        ((z + 116.0f) * (z + 116.0f)) / (34.0f * 34.0f)));

    const float shapedHeight =
        broadHills * 0.95f + rollingHills * 0.55f + ridgeBands * (0.40f + sideMask * 0.75f) +
        farMask * (0.40f +
                   ridgeNoise(sampleX * 0.18f - 3.0f, sampleZ * 0.18f + 9.0f, settings.seed + 47) *
                       1.05f) +
        sideMask * 0.78f - centralField * 0.48f - leftBasin * 0.82f - rightBasin * 0.60f - 0.62f;

    return shapedHeight * settings.terrainHeight - 2.6f;
}

float sampleAtmosphericTerrainHeight(const Scene& scene, float x, float z)
{
    return sampleAtmosphericTerrainHeight(scene.proceduralWorld, x, z);
}

float sampleAtmosphericTerrainSlope(const ProceduralWorldSettings& settings, float x, float z)
{
    const float offset = 3.0f;
    const float dx = sampleAtmosphericTerrainHeight(settings, x + offset, z) -
                     sampleAtmosphericTerrainHeight(settings, x - offset, z);
    const float dz = sampleAtmosphericTerrainHeight(settings, x, z + offset) -
                     sampleAtmosphericTerrainHeight(settings, x, z - offset);
    return std::sqrt(dx * dx + dz * dz) * 0.5f;
}

Vec3 sampleAtmosphericTerrainNormal(const ProceduralWorldSettings& settings, float x, float z)
{
    const float offset = 1.5f;
    const Vec3 tangentX{2.0f * offset,
                        sampleAtmosphericTerrainHeight(settings, x + offset, z) -
                            sampleAtmosphericTerrainHeight(settings, x - offset, z),
                        0.0f};
    const Vec3 tangentZ{0.0f,
                        sampleAtmosphericTerrainHeight(settings, x, z + offset) -
                            sampleAtmosphericTerrainHeight(settings, x, z - offset),
                        2.0f * offset};
    return normalize(cross(tangentZ, tangentX));
}

Vec3 sampleAtmosphericTerrainNormal(const Scene& scene, float x, float z)
{
    return sampleAtmosphericTerrainNormal(scene.proceduralWorld, x, z);
}

CollisionWorld buildCollisionWorld(const Scene& scene)
{
    CollisionWorld world{};

    scene.registry()
        .forEach<components::WorldObjectComponent, components::TransformComponent,
                 components::RenderMeshComponent>(
            [&](ecs::Entity, const components::WorldObjectComponent& object,
                const components::TransformComponent& transform,
                const components::RenderMeshComponent& renderMesh)
            {
                if (!isMovementSolidWorldObject(object) || renderMesh.mesh == nullptr)
                {
                    return;
                }

                const Geometry& geometry = renderMesh.mesh->geometry();
                if (geometry.vertices.empty())
                {
                    return;
                }

                Transform legacyTransform{};
                legacyTransform.position = transform.position;
                legacyTransform.rotation = transform.rotation;
                legacyTransform.scale = transform.scale;
                const Mat4 modelMatrix = legacyTransform.modelMatrix();

                auto appendTriangle =
                    [&](unsigned int index0, unsigned int index1, unsigned int index2)
                {
                    if (index0 >= geometry.vertices.size() || index1 >= geometry.vertices.size() ||
                        index2 >= geometry.vertices.size())
                    {
                        return;
                    }

                    const Vec3 a = transformPoint(modelMatrix, geometry.vertices[index0].position);
                    const Vec3 b = transformPoint(modelMatrix, geometry.vertices[index1].position);
                    const Vec3 c = transformPoint(modelMatrix, geometry.vertices[index2].position);
                    const Vec3 normal = normalize(cross(b - a, c - a));
                    if (length(normal) <= kSweepEpsilon)
                    {
                        return;
                    }

                    const Bounds bounds = makeTriangleBounds(a, b, c);
                    const std::size_t triangleIndex = world.triangles.size();
                    world.triangles.push_back(
                        CollisionTriangle{a, b, c, normal, bounds.minimum, bounds.maximum});

                    const int minCellX =
                        static_cast<int>(std::floor(bounds.minimum.x / world.cellSize));
                    const int maxCellX =
                        static_cast<int>(std::floor(bounds.maximum.x / world.cellSize));
                    const int minCellZ =
                        static_cast<int>(std::floor(bounds.minimum.z / world.cellSize));
                    const int maxCellZ =
                        static_cast<int>(std::floor(bounds.maximum.z / world.cellSize));

                    for (int cellZ = minCellZ; cellZ <= maxCellZ; ++cellZ)
                    {
                        for (int cellX = minCellX; cellX <= maxCellX; ++cellX)
                        {
                            world.cellMap[cellKey(cellX, cellZ)].push_back(triangleIndex);
                        }
                    }
                };

                if (!geometry.indices.empty())
                {
                    for (std::size_t index = 0; index + 2 < geometry.indices.size(); index += 3)
                    {
                        appendTriangle(geometry.indices[index], geometry.indices[index + 1],
                                       geometry.indices[index + 2]);
                    }
                }
                else
                {
                    for (unsigned int index = 0; index + 2 < geometry.vertices.size(); index += 3)
                    {
                        appendTriangle(index, index + 1, index + 2);
                    }
                }
            });

    return world;
}

bool collisionWorldCanOccupy(const CollisionWorld& world, const Vec3& feetPosition,
                             const CapsuleCollisionShape& shape, float skinWidth)
{
    const float radius = std::max(shape.radius - skinWidth, 0.01f);
    const ContactCandidate contact =
        findClosestCapsuleContact(world, feetPosition, shape, radius, radius + skinWidth);
    return !contact.hit || contact.separation >= -skinWidth;
}

SupportInfo queryGroundSupport(const CollisionWorld& world, const Vec3& feetPosition,
                               const CapsuleCollisionShape& shape, float maxSupportDistance,
                               float maxSlopeAngleDegrees)
{
    SupportInfo result{};
    const float minimumWalkableNormalY = walkableNormalY(maxSlopeAngleDegrees);
    const float radius = shape.radius;
    const Bounds bounds = expandBounds(capsuleBounds(feetPosition, shape, radius),
                                       Vec3{radius, maxSupportDistance + radius, radius});
    const Vec3 center = feetPosition + Vec3{0.0f, radius, 0.0f};

    float bestVerticalDistance = std::numeric_limits<float>::infinity();
    for (std::size_t triangleIndex : queryTriangleIndices(world, bounds))
    {
        const CollisionTriangle& triangle = world.triangles[triangleIndex];
        const Vec3 point = closestPointOnTriangle(center, triangle);
        const float distance = length(center - point);
        const float separation = distance - radius;
        if (separation > maxSupportDistance)
        {
            continue;
        }

        const Vec3 normal = contactNormal(center, point, triangle);
        if (normal.y < minimumWalkableNormalY)
        {
            continue;
        }

        const float verticalDistance = separation / std::max(normal.y, 0.05f);
        if (verticalDistance > maxSupportDistance)
        {
            continue;
        }

        if (!result.hit || verticalDistance < bestVerticalDistance ||
            (std::abs(verticalDistance - bestVerticalDistance) <= 0.0001f &&
             normal.y > result.normal.y))
        {
            bestVerticalDistance = verticalDistance;
            result.hit = true;
            result.walkable = true;
            result.distance = verticalDistance;
            result.height = feetPosition.y - verticalDistance;
            result.point = point;
            result.normal = normal;
            result.slopeAngleDegrees = slopeAngleDegreesFromNormal(normal);
        }
    }

    return result;
}

MotionResult sweepAndSlideCapsule(const CollisionWorld& world, const Vec3& startFeetPosition,
                                  const Vec3& displacement, const Vec3& velocity,
                                  const CapsuleCollisionShape& shape, float skinWidth,
                                  int maxIterations)
{
    MotionResult result{};
    result.position = startFeetPosition;
    result.velocity = velocity;

    const float radius = std::max(shape.radius - skinWidth, 0.01f);
    Vec3 position = startFeetPosition;
    Vec3 remaining = displacement;
    result.penetrationRecoveries =
        resolveCapsulePenetration(position, world, shape, radius, skinWidth);

    for (int iteration = 0; iteration < maxIterations; ++iteration)
    {
        result.iterations = iteration + 1;
        if (length(remaining) <= kSweepEpsilon)
        {
            break;
        }

        ContactCandidate hit{};
        Vec3 advancePosition = position;
        Vec3 advanceRemaining = remaining;
        for (int refinement = 0; refinement < 8; ++refinement)
        {
            hit = findEarliestApproachingContact(world, advancePosition, advanceRemaining, shape,
                                                 radius, skinWidth);
            if (!hit.hit)
            {
                advancePosition = advancePosition + advanceRemaining;
                advanceRemaining = Vec3{};
                break;
            }

            if (hit.separation <= skinWidth + kSweepEpsilon)
            {
                break;
            }

            const float approach = -dot(advanceRemaining, hit.normal);
            if (approach <= kSweepEpsilon)
            {
                advancePosition = advancePosition + advanceRemaining;
                advanceRemaining = Vec3{};
                break;
            }

            const float fraction = std::clamp((hit.separation - skinWidth) / approach, 0.0f, 1.0f);
            if (fraction >= 1.0f - kSweepEpsilon)
            {
                advancePosition = advancePosition + advanceRemaining;
                advanceRemaining = Vec3{};
                hit = ContactCandidate{};
                break;
            }

            advancePosition = advancePosition + advanceRemaining * fraction;
            advanceRemaining = advanceRemaining * (1.0f - fraction);
            if (fraction <= kSweepEpsilon)
            {
                break;
            }
        }

        if (!hit.hit)
        {
            position = advancePosition;
            remaining = advanceRemaining;
            break;
        }

        position = advancePosition;
        result.lastCollisionNormal = hit.normal;
        result.velocity = removeIntoSurfaceComponent(result.velocity, hit.normal);
        remaining = removeIntoSurfaceComponent(advanceRemaining, hit.normal);
        result.lastSurfaceMotion = remaining;
        result.penetrationRecoveries +=
            resolveCapsulePenetration(position, world, shape, radius, skinWidth);
        ++result.collisionCount;
    }

    result.position = position;
    result.residualMotionLength = length(remaining);
    result.exhaustedIterations = result.residualMotionLength > kSweepEpsilon;
    return result;
}
} // namespace engine
