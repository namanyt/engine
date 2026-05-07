#include "world/WorldNavigation.h"

#include "components/WorldComponents.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
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

bool hasSemantic(const engine::components::WorldObjectComponent& object,
                 engine::WorldObjectSemantic semantic)
{
    return (object.semantics & engine::toSemanticFlags(semantic)) != 0u;
}

bool isSolidWorldObject(const engine::components::WorldObjectComponent& object)
{
    if (!hasSemantic(object, engine::WorldObjectSemantic::Surface))
    {
        return false;
    }

    switch (object.kind)
    {
    case engine::WorldObjectKind::Ground:
    case engine::WorldObjectKind::Terrain:
    case engine::WorldObjectKind::Moon:
        return false;
    case engine::WorldObjectKind::Rock:
    case engine::WorldObjectKind::TreeTrunk:
    case engine::WorldObjectKind::TreeFoliage:
    case engine::WorldObjectKind::Tower:
    case engine::WorldObjectKind::Beacon:
    case engine::WorldObjectKind::Monolith:
    case engine::WorldObjectKind::Spire:
    case engine::WorldObjectKind::Marker:
        return true;
    }

    return false;
}

bool isStandableWorldObject(const engine::components::WorldObjectComponent& object)
{
    switch (object.kind)
    {
    case engine::WorldObjectKind::Rock:
    case engine::WorldObjectKind::Tower:
    case engine::WorldObjectKind::Monolith:
        return true;
    case engine::WorldObjectKind::Ground:
    case engine::WorldObjectKind::Terrain:
    case engine::WorldObjectKind::TreeTrunk:
    case engine::WorldObjectKind::TreeFoliage:
    case engine::WorldObjectKind::Beacon:
    case engine::WorldObjectKind::Spire:
    case engine::WorldObjectKind::Marker:
    case engine::WorldObjectKind::Moon:
        return false;
    }

    return false;
}

bool usesBoxCollision(const engine::components::WorldObjectComponent& object)
{
    switch (object.kind)
    {
    case engine::WorldObjectKind::Rock:
    case engine::WorldObjectKind::Tower:
    case engine::WorldObjectKind::Monolith:
    case engine::WorldObjectKind::Beacon:
        return true;
    case engine::WorldObjectKind::Ground:
    case engine::WorldObjectKind::Terrain:
    case engine::WorldObjectKind::TreeTrunk:
    case engine::WorldObjectKind::TreeFoliage:
    case engine::WorldObjectKind::Spire:
    case engine::WorldObjectKind::Marker:
    case engine::WorldObjectKind::Moon:
        return false;
    }

    return false;
}

float objectCollisionRadius(const engine::components::WorldObjectComponent& object,
                            const engine::components::TransformComponent& transform)
{
    const float baseRadius = std::max(transform.scale.x, transform.scale.z);

    switch (object.kind)
    {
    case engine::WorldObjectKind::TreeTrunk:
        return baseRadius * 0.58f;
    case engine::WorldObjectKind::TreeFoliage:
        return baseRadius * 0.46f;
    case engine::WorldObjectKind::Marker:
    case engine::WorldObjectKind::Beacon:
        return baseRadius * 0.44f;
    case engine::WorldObjectKind::Spire:
        return baseRadius * 0.54f;
    case engine::WorldObjectKind::Rock:
    case engine::WorldObjectKind::Tower:
    case engine::WorldObjectKind::Monolith:
        return baseRadius * 0.52f;
    default:
        return baseRadius * 0.5f;
    }
}

float objectHalfHeight(const engine::components::WorldObjectComponent& object,
                       const engine::components::TransformComponent& transform)
{
    switch (object.kind)
    {
    case engine::WorldObjectKind::TreeFoliage:
        return transform.scale.y * 0.42f;
    default:
        return transform.scale.y * 0.5f;
    }
}

float objectSupportRadius(const engine::components::WorldObjectComponent& object,
                          const engine::components::TransformComponent& transform)
{
    return objectCollisionRadius(object, transform) * 0.82f;
}

engine::Vec2 objectHalfExtents(const engine::components::WorldObjectComponent& object,
                               const engine::components::TransformComponent& transform)
{
    const float xExtent = std::max(transform.scale.x * 0.5f, 0.05f);
    const float zExtent = std::max(transform.scale.z * 0.5f, 0.05f);

    switch (object.kind)
    {
    case engine::WorldObjectKind::Beacon:
        return engine::Vec2{xExtent * 0.72f, zExtent * 0.72f};
    case engine::WorldObjectKind::Rock:
        return engine::Vec2{xExtent * 0.92f, zExtent * 0.92f};
    case engine::WorldObjectKind::Tower:
    case engine::WorldObjectKind::Monolith:
        return engine::Vec2{xExtent, zExtent};
    default:
        return engine::Vec2{xExtent, zExtent};
    }
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

SupportHeightResult sampleAtmosphericObjectSupportHeight(const Scene& scene, float x, float z,
                                                         float minimumHeight, float maximumHeight)
{
    SupportHeightResult result{};

    scene.registry().forEach<components::WorldObjectComponent, components::TransformComponent>(
        [&](ecs::Entity, const components::WorldObjectComponent& object,
            const components::TransformComponent& transform)
        {
            if (!isStandableWorldObject(object))
            {
                return;
            }

            const float top = transform.position.y + objectHalfHeight(object, transform);
            if (top < minimumHeight || top > maximumHeight)
            {
                return;
            }

            const float supportRadius = objectSupportRadius(object, transform);
            const Vec2 delta{x - transform.position.x, z - transform.position.z};
            if (usesBoxCollision(object))
            {
                const Vec2 halfExtents = objectHalfExtents(object, transform);
                if (std::abs(delta.x) > halfExtents.x || std::abs(delta.y) > halfExtents.y)
                {
                    return;
                }
            }
            else if (dot(delta, delta) > supportRadius * supportRadius)
            {
                return;
            }

            if (!result.hit || top > result.height)
            {
                result.hit = true;
                result.height = top;
            }
        });

    return result;
}

void resolveAtmosphericWorldCollision(const Scene& scene, const Vec3& previousFeetPosition,
                                      Vec3& feetPosition, const CylinderCollisionShape& shape,
                                      float maxStepUp)
{
    for (int pass = 0; pass < 3; ++pass)
    {
        bool adjusted = false;

        scene.registry().forEach<components::WorldObjectComponent, components::TransformComponent>(
            [&](ecs::Entity, const components::WorldObjectComponent& object,
                const components::TransformComponent& transform)
            {
                if (!isSolidWorldObject(object))
                {
                    return;
                }

                const float playerBottom = feetPosition.y;
                const float playerTop = feetPosition.y + shape.height;
                const float halfHeight = objectHalfHeight(object, transform);
                const float objectBottom = transform.position.y - halfHeight;
                const float objectTop = transform.position.y + halfHeight;
                if (playerTop <= objectBottom || playerBottom >= objectTop)
                {
                    return;
                }

                const Vec2 delta{feetPosition.x - transform.position.x,
                                 feetPosition.z - transform.position.z};
                if (isStandableWorldObject(object) &&
                    previousFeetPosition.y >= objectTop - maxStepUp &&
                    feetPosition.y <= objectTop + maxStepUp)
                {
                    if (usesBoxCollision(object))
                    {
                        const Vec2 halfExtents = objectHalfExtents(object, transform);
                        if (std::abs(delta.x) <= halfExtents.x + shape.radius &&
                            std::abs(delta.y) <= halfExtents.y + shape.radius)
                        {
                            feetPosition.y = std::max(feetPosition.y, objectTop);
                            return;
                        }
                    }
                    else
                    {
                        const float collisionRadius = objectCollisionRadius(object, transform);
                        const float minimumDistance = shape.radius + collisionRadius;
                        if (dot(delta, delta) <= minimumDistance * minimumDistance)
                        {
                            feetPosition.y = std::max(feetPosition.y, objectTop);
                            return;
                        }
                    }
                }

                if (usesBoxCollision(object))
                {
                    const Vec2 halfExtents = objectHalfExtents(object, transform);
                    const float overlapX = (halfExtents.x + shape.radius) - std::abs(delta.x);
                    const float overlapZ = (halfExtents.y + shape.radius) - std::abs(delta.y);
                    if (overlapX <= 0.0f || overlapZ <= 0.0f)
                    {
                        return;
                    }

                    if (overlapX < overlapZ)
                    {
                        feetPosition.x += (delta.x >= 0.0f ? overlapX : -overlapX) +
                                          (delta.x >= 0.0f ? 0.001f : -0.001f);
                    }
                    else
                    {
                        feetPosition.z += (delta.y >= 0.0f ? overlapZ : -overlapZ) +
                                          (delta.y >= 0.0f ? 0.001f : -0.001f);
                    }
                    adjusted = true;
                    return;
                }

                const float collisionRadius = objectCollisionRadius(object, transform);
                const float distanceSquared = dot(delta, delta);
                const float minimumDistance = shape.radius + collisionRadius;
                if (distanceSquared >= minimumDistance * minimumDistance)
                {
                    return;
                }

                const float distance = std::sqrt(std::max(distanceSquared, 0.0f));
                const Vec2 direction = distance > 0.0001f ? delta / distance : Vec2{1.0f, 0.0f};
                const float pushDistance = minimumDistance - distance + 0.001f;
                feetPosition.x += direction.x * pushDistance;
                feetPosition.z += direction.y * pushDistance;
                adjusted = true;
            });

        if (!adjusted)
        {
            break;
        }
    }
}
} // namespace engine
