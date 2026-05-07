#pragma once

#include "ecs/Entity.h"
#include "math/Types.h"
#include "world/Lighting.h"
#include "world/Material.h"
#include "world/Scene.h"

#include <string>

namespace engine
{
class Mesh;
}

namespace engine::components
{
struct NameComponent final
{
    std::string value;
};

struct TransformComponent final
{
    Vec3 position{};
    Vec3 rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct RenderMeshComponent final
{
    const Mesh* mesh = nullptr;
};

struct MaterialComponent final
{
    Material material{};
};

struct WorldObjectComponent final
{
    WorldObjectId id = WorldObjectId::None;
    WorldObjectKind kind = WorldObjectKind::Monolith;
    unsigned int semantics = toSemanticFlags(WorldObjectSemantic::None);
    bool castsShadows = true;
};

struct RayOccluderComponent final
{
    Vec3 centerOffset{};
    float radius = 0.0f;
};

struct LocalLightComponent final
{
    LocalLight light{};
};

struct VelocityComponent final
{
    Vec3 value{};
};

struct ColliderComponent final
{
    float radius = 0.42f;
    float height = 1.8f;
};

struct PlayerComponent final
{
    bool active = true;
};

struct CameraComponent final
{
    float eyeHeight = 1.64f;
    bool active = false;
    bool debugFreeCamera = false;
    float yawDegrees = -90.0f;
    float pitchDegrees = 0.0f;
    float fieldOfViewRadians = 0.78539816339f;
    float nearPlane = 0.1f;
};
} // namespace engine::components
