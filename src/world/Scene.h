#pragma once

#include "math/Transform.h"
#include "world/Lighting.h"
#include "world/Material.h"
#include "world/RayTracing.h"

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
    Tower,
    Beacon,
    Monolith,
    Spire,
    Marker,
    Moon,
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
    WorldObject& addObject(WorldObject object)
    {
        m_objects.push_back(std::move(object));
        return m_objects.back();
    }

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

  private:
    std::vector<WorldObject> m_objects;
};
} // namespace engine
