#pragma once

#include "ecs/Registry.h"
#include "math/Transform.h"
#include "world/Lighting.h"
#include "world/Material.h"
#include "world/RayTracing.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace engine
{
class Mesh;

enum class WorldObjectId
{
    None,
    Ground,
    DistantSpire,
    MarkerLeft,
    MarkerRight,
    Moon,
};

enum class WorldObjectKind
{
    Ground,
    Terrain,
    Rock,
    TreeTrunk,
    TreeFoliage,
    Tower,
    Beacon,
    Monolith,
    Spire,
    Marker,
    Moon,
};

struct ProceduralWorldSettings final
{
    int seed = 7;
    int terrainDensity = 52;
    float terrainScale = 54.0f;
    float terrainHeight = 12.0f;
    int treeCount = 72;
    bool regenerationRequested = false;
};

enum class WorldObjectSemantic : unsigned int
{
    None = 0,
    Surface = 1u << 0,
    Emissive = 1u << 1,
    RayOccluder = 1u << 2,
};

inline constexpr WorldObjectSemantic operator|(WorldObjectSemantic left, WorldObjectSemantic right)
{
    return static_cast<WorldObjectSemantic>(static_cast<unsigned int>(left) |
                                            static_cast<unsigned int>(right));
}

inline constexpr unsigned int toSemanticFlags(WorldObjectSemantic semantic)
{
    return static_cast<unsigned int>(semantic);
}

struct WorldObject final
{
    std::string debugName;
    WorldObjectId id = WorldObjectId::None;
    WorldObjectKind kind = WorldObjectKind::Monolith;
    unsigned int semantics = toSemanticFlags(WorldObjectSemantic::None);
    const Mesh* mesh = nullptr;
    Transform transform{};
    Material material{};
    bool castsShadows = true;

    bool hasSemantic(WorldObjectSemantic semantic) const noexcept
    {
        return (semantics & toSemanticFlags(semantic)) != 0u;
    }
};

class Scene final
{
  public:
    Scene();
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept;
    Scene& operator=(Scene&&) noexcept;

    WorldObject& addObject(WorldObject object)
    {
        m_objects.push_back(std::move(object));
        return m_objects.back();
    }

    ecs::Registry& registry() noexcept
    {
        return m_registry;
    }

    const ecs::Registry& registry() const noexcept
    {
        return m_registry;
    }

    Mesh& ownMesh(std::unique_ptr<Mesh> mesh);
    void clearRuntimeViews();
    void resetWorld();

    const std::vector<WorldObject>& objects() const noexcept
    {
        return m_objects;
    }

    std::vector<WorldObject>& objects() noexcept
    {
        return m_objects;
    }

    WorldObject* findObject(WorldObjectId id) noexcept
    {
        for (WorldObject& object : m_objects)
        {
            if (object.id == id)
            {
                return &object;
            }
        }

        return nullptr;
    }

    const WorldObject* findObject(WorldObjectId id) const noexcept
    {
        for (const WorldObject& object : m_objects)
        {
            if (object.id == id)
            {
                return &object;
            }
        }

        return nullptr;
    }

    FogSettings fog{};
    DirectionalLight sunLight{};
    std::vector<LocalLight> localLights{};
    ShadowSettings shadow{};
    SkyLight skyLight{};
    PostProcessSettings postProcess{};
    RayEvaluationSettings rayEvaluation{};
    DebugViewSettings debugView{};
    RayTracingScene rayTracingScene{};
    Color clearColor{0.05f, 0.08f, 0.11f, 1.0f};
    bool moonLightEnabled = true;
    bool sphereLightsEnabled = true;
    bool coneLightsEnabled = true;
    bool moonEmissiveEnabled = true;
    bool sphereEmissiveEnabled = true;
    bool coneEmissiveEnabled = true;
    bool moonMotionEnabled = true;
    float moonTimeOffset = 0.0f;
    Vec3 moonVisualPosition{};
    bool debugMoonVisualOverrideEnabled = false;
    Vec3 debugMoonVisualOverridePosition{};
    ProceduralWorldSettings proceduralWorld{};

  private:
    ecs::Registry m_registry;
    std::vector<WorldObject> m_objects;
    std::vector<std::unique_ptr<Mesh>> m_ownedMeshes;
};
} // namespace engine
