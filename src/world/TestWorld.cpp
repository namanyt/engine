#include "world/TestWorld.h"

#include "components/WorldComponents.h"
#include "systems/WorldEcsSystems.h"
#include "world/WorldNavigation.h"

#include "graphics/Mesh.h"
#include "math/Types.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace
{
constexpr float kPi = 3.14159265359f;
constexpr float kTerrainMinX = -128.0f;
constexpr float kTerrainMaxX = 128.0f;
constexpr float kTerrainNearZ = 34.0f;
constexpr float kTerrainFarZ = -212.0f;
constexpr float kMoonVisualDistance = 420.0f;
constexpr float kMoonVisualScale = 24.0f;

struct TreePlacement final
{
    engine::Vec3 position{};
    float trunkHeight = 0.0f;
    float crownHeight = 0.0f;
    float crownRadius = 0.0f;
    float yaw = 0.0f;
    bool dead = false;
};

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

engine::Vec3 triangleNormal(const engine::Vec3& a, const engine::Vec3& b, const engine::Vec3& c)
{
    const engine::Vec3 normal = engine::cross(b - a, c - a);
    const float magnitude = engine::length(normal);
    if (magnitude <= 0.0001f)
    {
        return engine::Vec3{0.0f, 1.0f, 0.0f};
    }

    return normal / magnitude;
}

void appendFlatTriangle(engine::Geometry& geometry, const engine::Vec3& a, const engine::Vec3& b,
                        const engine::Vec3& c, const engine::Vec2& uvA, const engine::Vec2& uvB,
                        const engine::Vec2& uvC)
{
    const engine::Vec3 normal = triangleNormal(a, b, c);
    const unsigned int baseIndex = static_cast<unsigned int>(geometry.vertices.size());

    geometry.vertices.push_back(engine::Vertex{a, normal, uvA});
    geometry.vertices.push_back(engine::Vertex{b, normal, uvB});
    geometry.vertices.push_back(engine::Vertex{c, normal, uvC});

    geometry.indices.push_back(baseIndex);
    geometry.indices.push_back(baseIndex + 1);
    geometry.indices.push_back(baseIndex + 2);
}

float sampleTerrainHeight(const engine::ProceduralWorldSettings& settings, float x, float z)
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

float sampleTerrainSlope(const engine::ProceduralWorldSettings& settings, float x, float z)
{
    const float offset = 3.0f;
    const float dx =
        sampleTerrainHeight(settings, x + offset, z) - sampleTerrainHeight(settings, x - offset, z);
    const float dz =
        sampleTerrainHeight(settings, x, z + offset) - sampleTerrainHeight(settings, x, z - offset);
    return std::sqrt(dx * dx + dz * dz) * 0.5f;
}

engine::Geometry buildTerrainGeometry(const engine::ProceduralWorldSettings& settings)
{
    const int cellsX = std::max(settings.terrainDensity, 20);
    const int cellsZ = std::max(settings.terrainDensity + settings.terrainDensity / 4, 24);
    engine::Geometry geometry{};
    geometry.vertices.reserve(static_cast<std::size_t>(cellsX * cellsZ * 6));
    geometry.indices.reserve(static_cast<std::size_t>(cellsX * cellsZ * 6));

    for (int zIndex = 0; zIndex < cellsZ; ++zIndex)
    {
        const float z0Factor = static_cast<float>(zIndex) / static_cast<float>(cellsZ);
        const float z1Factor = static_cast<float>(zIndex + 1) / static_cast<float>(cellsZ);
        const float z0 = lerp(kTerrainNearZ, kTerrainFarZ, z0Factor);
        const float z1 = lerp(kTerrainNearZ, kTerrainFarZ, z1Factor);

        for (int xIndex = 0; xIndex < cellsX; ++xIndex)
        {
            const float x0Factor = static_cast<float>(xIndex) / static_cast<float>(cellsX);
            const float x1Factor = static_cast<float>(xIndex + 1) / static_cast<float>(cellsX);
            const float x0 = lerp(kTerrainMinX, kTerrainMaxX, x0Factor);
            const float x1 = lerp(kTerrainMinX, kTerrainMaxX, x1Factor);

            const engine::Vec3 p00{x0, sampleTerrainHeight(settings, x0, z0), z0};
            const engine::Vec3 p10{x1, sampleTerrainHeight(settings, x1, z0), z0};
            const engine::Vec3 p01{x0, sampleTerrainHeight(settings, x0, z1), z1};
            const engine::Vec3 p11{x1, sampleTerrainHeight(settings, x1, z1), z1};

            appendFlatTriangle(geometry, p00, p10, p11, engine::Vec2{x0Factor, z0Factor},
                               engine::Vec2{x1Factor, z0Factor}, engine::Vec2{x1Factor, z1Factor});
            appendFlatTriangle(geometry, p00, p11, p01, engine::Vec2{x0Factor, z0Factor},
                               engine::Vec2{x1Factor, z1Factor}, engine::Vec2{x0Factor, z1Factor});
        }
    }

    return geometry;
}

engine::Vec3 orbitingMoonOrbitOffset(float timeSeconds)
{
    const float angle = timeSeconds * 0.12f;
    return engine::Vec3{std::cos(angle) * 96.0f, 42.0f + std::sin(angle * 0.65f) * 12.0f,
                        -74.0f + std::sin(angle) * 76.0f};
}

engine::Vec3 computeMoonLightDirection(const engine::Scene& scene, float timeSeconds)
{
    return engine::normalize(scene.shadow.focusPoint - orbitingMoonOrbitOffset(timeSeconds));
}

engine::Vec3 computeMoonVisualPosition(const engine::Vec3& cameraPosition,
                                       const engine::Vec3& lightDirection)
{
    return cameraPosition - engine::normalize(lightDirection) * kMoonVisualDistance;
}

float rayOccluderRadius(const engine::Vec3& scale, float multiplier = 1.0f)
{
    return std::max(scale.x, std::max(scale.y, scale.z)) * multiplier;
}

void appendRayOccluder(engine::Scene& scene, const engine::Vec3& center, float radius)
{
    engine::systems::spawnRayOccluderEntity(scene, "TerrainOccluder", center, radius);
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

    engine::systems::spawnWorldObjectEntity(
        scene, debugName, id, kind, engine::toSemanticFlags(semantics), mesh, material, position,
        rotation, scale, true,
        occluderScale > 0.0f ? rayOccluderRadius(scale, occluderScale) : 0.0f);
}

void addLocalPointLight(engine::Scene& scene, const engine::Vec3& position,
                        const engine::Vec3& color, float intensity, float range,
                        engine::LocalLightGroup group)
{
    engine::systems::spawnLocalLightEntity(scene, "LocalPointLight", position, color, intensity,
                                           range, group);
}

engine::Vec3 terrainPosition(const engine::ProceduralWorldSettings& settings, float x, float z,
                             float yOffset = 0.0f)
{
    return engine::Vec3{x, sampleTerrainHeight(settings, x, z) + yOffset, z};
}

void addTerrainOccluders(engine::Scene& scene, const engine::ProceduralWorldSettings& settings)
{
    const std::array<engine::Vec3, 8> occluders{
        engine::Vec3{-82.0f, 0.0f, -28.0f},  engine::Vec3{78.0f, 0.0f, -34.0f},
        engine::Vec3{-54.0f, 0.0f, -72.0f},  engine::Vec3{30.0f, 0.0f, -112.0f},
        engine::Vec3{-70.0f, 0.0f, -126.0f}, engine::Vec3{68.0f, 0.0f, -164.0f},
        engine::Vec3{-16.0f, 0.0f, -176.0f}, engine::Vec3{18.0f, 0.0f, -202.0f},
    };
    const std::array<float, 8> radii{34.0f, 30.0f, 24.0f, 26.0f, 28.0f, 30.0f, 34.0f, 38.0f};

    for (std::size_t index = 0; index < occluders.size(); ++index)
    {
        const engine::Vec3 center =
            terrainPosition(settings, occluders[index].x, occluders[index].z, radii[index] * 0.15f);
        appendRayOccluder(scene, center, radii[index]);
    }
}

std::vector<TreePlacement> buildTreePlacements(const engine::ProceduralWorldSettings& settings)
{
    const std::array<engine::Vec2, 5> clusterCenters{
        engine::Vec2{-40.0f, -30.0f}, engine::Vec2{36.0f, -64.0f},  engine::Vec2{-48.0f, -108.0f},
        engine::Vec2{18.0f, -146.0f}, engine::Vec2{60.0f, -174.0f},
    };
    const std::array<float, 5> clusterRadii{18.0f, 22.0f, 24.0f, 28.0f, 22.0f};

    std::vector<TreePlacement> placements;
    const int targetCount = std::max(settings.treeCount, 0);
    placements.reserve(static_cast<std::size_t>(targetCount + std::min(6, targetCount / 8)));

    for (int attempt = 0;
         attempt < targetCount * 14 && static_cast<int>(placements.size()) < targetCount; ++attempt)
    {
        const int clusterIndex = static_cast<int>(hash01(attempt, 17, settings.seed) * 5.0f) %
                                 static_cast<int>(clusterCenters.size());
        const engine::Vec2 center{
            clusterCenters[clusterIndex].x + hashSigned(clusterIndex, 31, settings.seed) * 8.0f,
            clusterCenters[clusterIndex].y + hashSigned(clusterIndex, 47, settings.seed) * 10.0f,
        };

        const float angle = hash01(attempt, 53, settings.seed) * kPi * 2.0f;
        const float radius =
            clusterRadii[clusterIndex] * std::sqrt(hash01(attempt, 59, settings.seed));
        const float x = center.x + std::cos(angle) * radius *
                                       lerp(0.72f, 1.12f, hash01(attempt, 61, settings.seed));
        const float z = center.y + std::sin(angle) * radius *
                                       lerp(0.68f, 1.20f, hash01(attempt, 67, settings.seed));

        if (std::abs(x) < 17.0f && z > -78.0f && z < 18.0f)
        {
            continue;
        }

        if ((x > -20.0f && x < 18.0f) && (z < -112.0f && z > -158.0f))
        {
            continue;
        }

        const float slope = sampleTerrainSlope(settings, x, z);
        if (slope > 1.8f + settings.terrainHeight * 0.06f)
        {
            continue;
        }

        const float y = sampleTerrainHeight(settings, x, z);
        const bool deadTree = hash01(attempt, 71, settings.seed) < (z < -128.0f ? 0.26f : 0.14f);
        const float trunkHeight = deadTree ? lerp(3.2f, 5.6f, hash01(attempt, 73, settings.seed))
                                           : lerp(2.5f, 4.6f, hash01(attempt, 73, settings.seed));
        const float crownHeight =
            deadTree ? 0.0f : lerp(4.6f, 8.2f, hash01(attempt, 79, settings.seed));
        const float crownRadius =
            deadTree ? 0.0f : lerp(1.5f, 3.2f, hash01(attempt, 83, settings.seed));

        placements.push_back(TreePlacement{
            engine::Vec3{x, y, z},
            trunkHeight,
            crownHeight,
            crownRadius,
            hash01(attempt, 89, settings.seed) * kPi * 2.0f,
            deadTree,
        });
    }

    const int silhouetteCount = std::min(6, targetCount / 8);
    for (int index = 0; index < silhouetteCount; ++index)
    {
        const float t = silhouetteCount > 1
                            ? static_cast<float>(index) / static_cast<float>(silhouetteCount - 1)
                            : 0.5f;
        const float x = lerp(-76.0f, 74.0f, t) + hashSigned(index, 101, settings.seed) * 6.0f;
        const float z = -132.0f - static_cast<float>(index) * 13.0f +
                        hashSigned(index, 109, settings.seed) * 5.0f;
        const float y = sampleTerrainHeight(settings, x, z);
        placements.push_back(TreePlacement{
            engine::Vec3{x, y, z},
            4.2f + hash01(index, 113, settings.seed) * 2.0f,
            6.0f + hash01(index, 127, settings.seed) * 2.8f,
            2.0f + hash01(index, 131, settings.seed) * 1.2f,
            hash01(index, 137, settings.seed) * kPi * 2.0f,
            false,
        });
    }

    return placements;
}

void addProceduralTree(engine::Scene& scene, const engine::TestWorldAssets& assets,
                       const TreePlacement& tree, const engine::Material& trunkMaterial,
                       const engine::Material& foliageMaterial,
                       const engine::Material& deadTreeMaterial)
{
    const engine::Vec3 trunkScale{tree.dead ? 0.24f : 0.30f, tree.trunkHeight,
                                  tree.dead ? 0.24f : 0.30f};
    addOccludingObject(scene, engine::WorldObjectId::None, engine::WorldObjectKind::TreeTrunk,
                       engine::WorldObjectSemantic::Surface |
                           engine::WorldObjectSemantic::RayOccluder,
                       tree.dead ? "DeadTreeTrunk" : "TreeTrunk", assets.cylinder,
                       tree.dead ? deadTreeMaterial : trunkMaterial,
                       tree.position + engine::Vec3{0.0f, tree.trunkHeight * 0.5f, 0.0f},
                       engine::Vec3{0.04f, tree.yaw, tree.dead ? 0.05f : 0.0f}, trunkScale,
                       tree.dead ? 1.30f : 1.18f);

    if (tree.dead)
    {
        for (int branchIndex = 0; branchIndex < 2; ++branchIndex)
        {
            const float branchSign = branchIndex == 0 ? -1.0f : 1.0f;
            addOccludingObject(
                scene, engine::WorldObjectId::None, engine::WorldObjectKind::TreeTrunk,
                engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
                "DeadTreeBranch", assets.cylinder, deadTreeMaterial,
                tree.position + engine::Vec3{branchSign * 0.55f, tree.trunkHeight * 0.72f, 0.0f},
                engine::Vec3{0.0f, tree.yaw, branchSign * 1.05f}, engine::Vec3{0.09f, 1.2f, 0.09f},
                0.66f);
        }

        return;
    }

    const int layerCount = tree.crownHeight > 6.6f ? 3 : 2;
    for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        const float layerT =
            static_cast<float>(layerIndex) / static_cast<float>(std::max(layerCount - 1, 1));
        const float layerHeight = tree.crownHeight * lerp(0.78f, 0.46f, layerT);
        const float layerRadius = tree.crownRadius * lerp(1.0f, 0.62f, layerT);
        const float baseHeight = tree.position.y + tree.trunkHeight - tree.crownHeight * 0.08f +
                                 tree.crownHeight * 0.28f * layerT;

        addOccludingObject(
            scene, engine::WorldObjectId::None, engine::WorldObjectKind::TreeFoliage,
            engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
            "TreeFoliage", assets.cone, foliageMaterial,
            engine::Vec3{tree.position.x, baseHeight + layerHeight * 0.5f, tree.position.z},
            engine::Vec3{0.0f, tree.yaw + layerT * 0.18f, 0.0f},
            engine::Vec3{layerRadius, layerHeight, layerRadius}, 0.88f);
    }
}

void addTerrainRocks(engine::Scene& scene, const engine::TestWorldAssets& assets,
                     const engine::ProceduralWorldSettings& settings,
                     const engine::Material& rockMaterial, const engine::Material& wetStoneMaterial)
{
    const std::array<engine::Vec3, 8> rockAnchors{
        engine::Vec3{-56.0f, 0.0f, -18.0f},  engine::Vec3{28.0f, 0.0f, -46.0f},
        engine::Vec3{-24.0f, 0.0f, -72.0f},  engine::Vec3{40.0f, 0.0f, -98.0f},
        engine::Vec3{-70.0f, 0.0f, -118.0f}, engine::Vec3{58.0f, 0.0f, -138.0f},
        engine::Vec3{-18.0f, 0.0f, -176.0f}, engine::Vec3{74.0f, 0.0f, -188.0f},
    };
    const std::array<engine::Vec3, 8> scales{
        engine::Vec3{3.6f, 2.4f, 3.2f}, engine::Vec3{2.8f, 1.6f, 4.4f},
        engine::Vec3{4.6f, 2.0f, 3.8f}, engine::Vec3{3.2f, 2.8f, 3.2f},
        engine::Vec3{5.2f, 3.0f, 4.2f}, engine::Vec3{4.0f, 2.2f, 5.0f},
        engine::Vec3{6.2f, 2.6f, 6.2f}, engine::Vec3{4.0f, 2.0f, 5.6f},
    };

    for (std::size_t index = 0; index < rockAnchors.size(); ++index)
    {
        const bool wetStone = index == 1 || index == 2 || index == 6;
        const engine::Vec3 position = terrainPosition(
            settings, rockAnchors[index].x, rockAnchors[index].z, scales[index].y * 0.5f - 0.1f);
        addOccludingObject(
            scene, engine::WorldObjectId::None, engine::WorldObjectKind::Rock,
            engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
            wetStone ? "WetStone" : "RockOutcrop", index % 3 == 0 ? assets.pyramid : assets.cube,
            wetStone ? wetStoneMaterial : rockMaterial, position,
            engine::Vec3{0.0f, hash01(static_cast<int>(index), 149, settings.seed), 0.0f},
            scales[index], 0.92f);
    }
}

void addLandmarks(engine::Scene& scene, const engine::TestWorldAssets& assets,
                  const engine::ProceduralWorldSettings& settings,
                  const engine::Material& stoneMaterial, const engine::Material& absorptiveMaterial,
                  const engine::Material& metallicMaterial,
                  const engine::Material& emissiveBeaconMaterial,
                  const engine::Material& markerMaterial)
{
    addOccludingObject(scene, engine::WorldObjectId::DistantSpire, engine::WorldObjectKind::Spire,
                       engine::WorldObjectSemantic::Surface |
                           engine::WorldObjectSemantic::RayOccluder,
                       "DistantSpire", assets.pyramid, metallicMaterial,
                       terrainPosition(settings, -6.0f, -196.0f, 18.0f),
                       engine::Vec3{0.0f, 0.34f, 0.0f}, engine::Vec3{13.0f, 18.0f, 13.0f}, 0.96f);

    const std::array<engine::Vec3, 4> monolithAnchors{
        engine::Vec3{-48.0f, 0.0f, -148.0f},
        engine::Vec3{-18.0f, 0.0f, -166.0f},
        engine::Vec3{22.0f, 0.0f, -154.0f},
        engine::Vec3{56.0f, 0.0f, -176.0f},
    };
    const std::array<engine::Vec3, 4> monolithScales{
        engine::Vec3{4.4f, 10.0f, 4.4f},
        engine::Vec3{3.8f, 13.0f, 3.8f},
        engine::Vec3{4.6f, 11.0f, 4.6f},
        engine::Vec3{5.2f, 14.0f, 5.2f},
    };

    for (std::size_t index = 0; index < monolithAnchors.size(); ++index)
    {
        addOccludingObject(
            scene, engine::WorldObjectId::None, engine::WorldObjectKind::Monolith,
            engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::RayOccluder,
            "DistantMonolith", assets.cube, index == 1 ? stoneMaterial : absorptiveMaterial,
            terrainPosition(settings, monolithAnchors[index].x, monolithAnchors[index].z,
                            monolithScales[index].y * 0.5f),
            engine::Vec3{0.0f, hash01(static_cast<int>(index), 181, settings.seed), 0.0f},
            monolithScales[index], 0.86f);
    }

    const engine::Vec3 leftMarkerBase = terrainPosition(settings, -22.0f, -58.0f, 2.8f);
    const engine::Vec3 rightMarkerBase = terrainPosition(settings, 26.0f, -126.0f, 3.4f);
    addOccludingObject(scene, engine::WorldObjectId::MarkerLeft, engine::WorldObjectKind::Marker,
                       engine::WorldObjectSemantic::Surface |
                           engine::WorldObjectSemantic::Emissive |
                           engine::WorldObjectSemantic::RayOccluder,
                       "FogMarkerLeft", assets.cone, markerMaterial, leftMarkerBase,
                       engine::Vec3{0.0f, 0.08f, 0.0f}, engine::Vec3{2.4f, 5.6f, 2.4f}, 0.82f);
    addOccludingObject(scene, engine::WorldObjectId::MarkerRight, engine::WorldObjectKind::Marker,
                       engine::WorldObjectSemantic::Surface |
                           engine::WorldObjectSemantic::Emissive |
                           engine::WorldObjectSemantic::RayOccluder,
                       "FogMarkerRight", assets.cone, markerMaterial, rightMarkerBase,
                       engine::Vec3{0.0f, -0.14f, 0.0f}, engine::Vec3{2.8f, 6.2f, 2.8f}, 0.82f);
    addLocalPointLight(scene, leftMarkerBase + engine::Vec3{0.0f, 3.2f, 0.0f},
                       engine::Vec3{0.24f, 0.54f, 0.52f}, 1.18f, 18.0f,
                       engine::LocalLightGroup::Cone);
    addLocalPointLight(scene, rightMarkerBase + engine::Vec3{0.0f, 3.8f, 0.0f},
                       engine::Vec3{0.18f, 0.48f, 0.46f}, 1.22f, 20.0f,
                       engine::LocalLightGroup::Cone);

    const std::array<engine::Vec3, 2> beaconBases{
        terrainPosition(settings, 12.0f, -92.0f, 2.1f),
        terrainPosition(settings, -34.0f, -132.0f, 2.5f),
    };
    const std::array<engine::Vec3, 2> beaconPedestalScales{
        engine::Vec3{1.8f, 4.2f, 1.8f},
        engine::Vec3{2.0f, 5.0f, 2.0f},
    };

    for (std::size_t index = 0; index < beaconBases.size(); ++index)
    {
        addOccludingObject(scene, engine::WorldObjectId::None, engine::WorldObjectKind::Tower,
                           engine::WorldObjectSemantic::Surface |
                               engine::WorldObjectSemantic::RayOccluder,
                           "BeaconPedestal", assets.cylinder, stoneMaterial, beaconBases[index],
                           engine::Vec3{0.0f, 0.0f, 0.0f}, beaconPedestalScales[index], 0.78f);

        const engine::Vec3 beaconPosition =
            beaconBases[index] + engine::Vec3{0.0f, beaconPedestalScales[index].y + 1.1f, 0.0f};
        addOccludingObject(
            scene, engine::WorldObjectId::None, engine::WorldObjectKind::Beacon,
            engine::WorldObjectSemantic::Surface | engine::WorldObjectSemantic::Emissive,
            "BeaconCore", assets.sphere, emissiveBeaconMaterial, beaconPosition,
            engine::Vec3{0.0f, 0.0f, 0.0f},
            index == 0 ? engine::Vec3{1.2f, 1.2f, 1.2f} : engine::Vec3{1.0f, 1.0f, 1.0f});
        addLocalPointLight(scene, beaconPosition, engine::Vec3{1.00f, 0.76f, 0.30f},
                           index == 0 ? 1.72f : 1.34f, index == 0 ? 18.0f : 15.0f,
                           engine::LocalLightGroup::Sphere);
    }
}

void applyWorldDefaults(engine::Scene& scene)
{
    scene.clearColor = engine::Color{0.006f, 0.008f, 0.013f, 1.0f};
    scene.fog =
        engine::FogSettings{engine::Vec3{0.046f, 0.054f, 0.072f}, 0.026f, 27.53f, 1.15f, 172.6f};
    scene.sunLight = engine::DirectionalLight{
        engine::normalize(engine::Vec3{-0.34f, -1.0f, -0.24f}),
        engine::Vec3{0.54f, 0.62f, 0.78f},
        0.96f,
    };
    scene.shadow = engine::ShadowSettings{
        2048, 104.0f, 1.0f, 280.0f, 0.00030f, 0.006f, 0.68f, engine::Vec3{0.0f, 8.0f, -84.0f}};
    scene.skyLight = engine::SkyLight{
        engine::Vec3{0.078f, 0.094f, 0.122f},
        engine::Vec3{0.010f, 0.016f, 0.030f},
        engine::Vec3{0.022f, 0.020f, 0.022f},
        0.34f,
    };
    scene.postProcess.exposure = 1.031f;
    scene.postProcess.bloomThreshold = 0.90f;
    scene.postProcess.bloomIntensity = 0.663f;
    scene.postProcess.gamma = 2.2f;
    scene.postProcess.contrast = 1.033f;
    scene.postProcess.vignetteStrength = 0.40f;
    scene.postProcess.saturation = 1.25f;
    scene.postProcess.midtoneLift = 0.080f;

    scene.rayEvaluation.shadowStrength = 0.12f;
    scene.rayEvaluation.scatteringStrength = 2.3f;
    scene.rayEvaluation.volumetricLightIntensity = 3.6f;
    scene.rayEvaluation.directionalLightAngularRadius = 0.004f;
    scene.rayEvaluation.stepLength = 10.0f;
    scene.rayEvaluation.maxDistance = 1500.0f;
    scene.rayEvaluation.maxSteps = 256;
    scene.rayEvaluation.extinctionStrength = 1.8f;
    scene.rayEvaluation.atmosphericAmbientFloor = 0.050f;
    scene.rayEvaluation.nearFieldHaze = 0.25f;
    scene.rayEvaluation.phaseAnisotropy = 0.42f;
    scene.rayEvaluation.jitterStrength = 1.0f;
    scene.rayEvaluation.stepDistributionExponent = 3.0f;
    scene.rayEvaluation.temporalJitterScale = 0.0f;
}

void rebuildAtmosphericWorld(engine::Scene& scene, const engine::TestWorldAssets& assets)
{
    scene.resetWorld();

    const engine::Material terrainMaterial =
        engine::makeWorldMaterial(assets.shader, engine::MaterialCategory::DenseAbsorptive,
                                  engine::Vec3{0.052f, 0.056f, 0.062f}, engine::Vec3{}, 0.0f, 0.96f,
                                  0.02f, 0.10f, 0.04f, 0.58f);
    const engine::Material rockMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::MatteStone, engine::Vec3{0.34f, 0.34f, 0.36f},
        engine::Vec3{}, 0.0f, 0.92f, 0.02f, 0.16f, 0.16f, 0.82f);
    const engine::Material wetStoneMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::WetReflective, engine::Vec3{0.12f, 0.18f, 0.22f},
        engine::Vec3{}, 0.0f, 0.18f, 0.08f, 0.88f, 0.04f, 1.30f);
    const engine::Material trunkMaterial =
        engine::makeWorldMaterial(assets.shader, engine::MaterialCategory::DenseAbsorptive,
                                  engine::Vec3{0.054f, 0.050f, 0.060f}, engine::Vec3{}, 0.0f, 0.94f,
                                  0.0f, 0.08f, 0.0f, 0.40f);
    const engine::Material deadTreeMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::MatteStone, engine::Vec3{0.14f, 0.14f, 0.15f},
        engine::Vec3{}, 0.0f, 0.98f, 0.0f, 0.06f, 0.0f, 0.34f);
    const engine::Material foliageMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::FogReactive, engine::Vec3{0.14f, 0.20f, 0.18f},
        engine::Vec3{0.02f, 0.05f, 0.04f}, 0.24f, 0.58f, 0.02f, 0.28f, 0.24f, 1.38f);
    const engine::Material markerMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::FogReactive, engine::Vec3{0.18f, 0.28f, 0.24f},
        engine::Vec3{0.08f, 0.16f, 0.15f}, 1.28f, 0.50f, 0.04f, 0.42f, 0.18f, 1.88f);
    const engine::Material metallicMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::MetallicStructure,
        engine::Vec3{0.42f, 0.48f, 0.54f}, engine::Vec3{}, 0.0f, 0.24f, 0.84f, 0.92f, 0.04f, 1.18f);
    const engine::Material emissiveBeaconMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::EmissiveBeacon, engine::Vec3{0.32f, 0.28f, 0.20f},
        engine::Vec3{1.00f, 0.76f, 0.28f}, 7.8f, 0.12f, 0.16f, 0.48f, 0.06f, 1.82f);
    const engine::Material absorptiveMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::DenseAbsorptive, engine::Vec3{0.07f, 0.07f, 0.08f},
        engine::Vec3{}, 0.0f, 0.96f, 0.0f, 0.06f, 0.0f, 0.28f);
    const engine::Material moonMaterial = engine::makeWorldMaterial(
        assets.shader, engine::MaterialCategory::CelestialBody, engine::Vec3{0.82f, 0.88f, 1.00f},
        engine::Vec3{0.78f, 0.84f, 1.00f}, 14.0f, 0.08f, 0.0f, 0.10f, 0.18f, 2.10f);

    if (assets.shader != nullptr)
    {
        engine::Mesh& terrainMesh = scene.ownMesh(
            std::make_unique<engine::Mesh>(buildTerrainGeometry(scene.proceduralWorld)));
        addOccludingObject(scene, engine::WorldObjectId::Ground, engine::WorldObjectKind::Terrain,
                           engine::WorldObjectSemantic::Surface |
                               engine::WorldObjectSemantic::RayOccluder,
                           "Terrain", &terrainMesh, terrainMaterial, engine::Vec3{0.0f, 0.0f, 0.0f},
                           engine::Vec3{0.0f, 0.0f, 0.0f}, engine::Vec3{1.0f, 1.0f, 1.0f});
    }

    addTerrainOccluders(scene, scene.proceduralWorld);

    addTerrainRocks(scene, assets, scene.proceduralWorld, rockMaterial, wetStoneMaterial);
    addLandmarks(scene, assets, scene.proceduralWorld, rockMaterial, absorptiveMaterial,
                 metallicMaterial, emissiveBeaconMaterial, markerMaterial);

    for (const TreePlacement& tree : buildTreePlacements(scene.proceduralWorld))
    {
        addProceduralTree(scene, assets, tree, trunkMaterial, foliageMaterial, deadTreeMaterial);
    }

    if (assets.sphere != nullptr)
    {
        engine::systems::spawnWorldObjectEntity(
            scene, "Moon", engine::WorldObjectId::Moon, engine::WorldObjectKind::Moon,
            engine::toSemanticFlags(engine::WorldObjectSemantic::Emissive), assets.sphere,
            moonMaterial, engine::Vec3{0.0f, 0.0f, 0.0f}, engine::Vec3{0.0f, 0.0f, 0.0f},
            engine::Vec3{kMoonVisualScale, kMoonVisualScale, kMoonVisualScale}, false);
    }

    scene.proceduralWorld.regenerationRequested = false;
    engine::systems::syncLegacySceneFromEcs(scene);
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
    applyWorldDefaults(scene);
    scene.proceduralWorld.regenerationRequested = true;
    syncAtmosphericTestWorld(scene, assets);
    return scene;
}

void syncAtmosphericTestWorld(Scene& scene, const TestWorldAssets& assets)
{
    if (!scene.proceduralWorld.regenerationRequested && !scene.objects().empty())
    {
        return;
    }

    rebuildAtmosphericWorld(scene, assets);
}

void updateAtmosphericWorldLighting(Scene& scene, float timeSeconds)
{
    const float effectiveTime =
        scene.moonMotionEnabled ? timeSeconds + scene.moonTimeOffset : scene.moonTimeOffset;
    const Vec3 moonDirection = computeMoonLightDirection(scene, effectiveTime);
    const bool anyLightingEnabled =
        scene.moonLightEnabled || scene.sphereLightsEnabled || scene.coneLightsEnabled;
    const bool anyEmissiveEnabled =
        scene.moonEmissiveEnabled || scene.sphereEmissiveEnabled || scene.coneEmissiveEnabled;

    if (const ecs::Entity moonEntity = systems::findWorldObjectEntity(scene, WorldObjectId::Moon);
        moonEntity != ecs::kInvalidEntity)
    {
        if (components::MaterialComponent* material =
                scene.registry().tryGet<components::MaterialComponent>(moonEntity);
            material != nullptr)
        {
            material->material.emissiveStrength =
                scene.moonEmissiveEnabled ? material->material.baseEmissiveStrength : 0.0f;
        }
    }

    scene.registry().forEach<components::WorldObjectComponent, components::MaterialComponent>(
        [&](ecs::Entity, const components::WorldObjectComponent& object,
            components::MaterialComponent& material)
        {
            if (object.kind == WorldObjectKind::Beacon)
            {
                material.material.emissiveStrength =
                    scene.sphereEmissiveEnabled ? material.material.baseEmissiveStrength : 0.0f;
            }
            else if (object.kind == WorldObjectKind::Marker)
            {
                material.material.emissiveStrength =
                    scene.coneEmissiveEnabled ? material.material.baseEmissiveStrength : 0.0f;
            }
        });

    scene.registry().forEach<components::LocalLightComponent>(
        [&](ecs::Entity, components::LocalLightComponent& light)
        {
            switch (light.light.group)
            {
            case LocalLightGroup::Sphere:
                light.light.intensity =
                    scene.sphereLightsEnabled ? light.light.baseIntensity : 0.0f;
                break;
            case LocalLightGroup::Cone:
                light.light.intensity = scene.coneLightsEnabled ? light.light.baseIntensity : 0.0f;
                break;
            case LocalLightGroup::Moon:
                light.light.intensity = 0.0f;
                light.light.enabled = false;
                break;
            }
        });

    scene.sunLight.direction = normalize(moonDirection);
    scene.sunLight.color = Vec3{0.58f, 0.66f, 0.84f};
    scene.sunLight.intensity = scene.moonLightEnabled ? 0.96f : 0.0f;
    scene.skyLight.intensity = scene.moonLightEnabled ? 0.34f : 0.0f;
    scene.clearColor = (anyLightingEnabled || anyEmissiveEnabled)
                           ? Color{0.006f, 0.008f, 0.013f, 1.0f}
                           : Color{0.0f, 0.0f, 0.0f, 1.0f};
    scene.fog.color = anyLightingEnabled ? Vec3{0.046f, 0.054f, 0.072f} : Vec3{0.0f, 0.0f, 0.0f};
    scene.rayEvaluation.shadowStrength = anyLightingEnabled ? 0.12f : 0.0f;
    if (!scene.moonLightEnabled && !scene.sphereLightsEnabled && !scene.coneLightsEnabled)
    {
        scene.skyLight.intensity = 0.0f;
    }

    systems::syncLegacySceneFromEcs(scene);

    if (scene.localLights.size() > 8)
    {
        scene.localLights.resize(8);
    }
}

void syncAtmosphericMoonVisual(Scene& scene, const Vec3& cameraPosition)
{
    const Vec3 derivedMoonPosition =
        computeMoonVisualPosition(cameraPosition, scene.sunLight.direction);
    const Vec3 moonPosition = scene.debugMoonVisualOverrideEnabled
                                  ? scene.debugMoonVisualOverridePosition
                                  : derivedMoonPosition;
    scene.moonVisualPosition = moonPosition;

    if (const ecs::Entity moonEntity = systems::findWorldObjectEntity(scene, WorldObjectId::Moon);
        moonEntity != ecs::kInvalidEntity)
    {
        if (components::TransformComponent* transform =
                scene.registry().tryGet<components::TransformComponent>(moonEntity);
            transform != nullptr)
        {
            transform->position = moonPosition;
            transform->scale = Vec3{kMoonVisualScale, kMoonVisualScale, kMoonVisualScale};
        }
    }

    if (WorldObject* moonObject = scene.findObject(WorldObjectId::Moon); moonObject != nullptr)
    {
        moonObject->transform.position = moonPosition;
        moonObject->transform.scale = Vec3{kMoonVisualScale, kMoonVisualScale, kMoonVisualScale};
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
