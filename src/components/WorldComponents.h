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
enum class LightType
{
    Directional,
    Point,
    EmissiveVolume,
};

enum class ColliderType
{
    Capsule,
    CylinderProxy,
    BoxProxy,
    TriangleMesh,
};

struct NameComponent final
{
    std::string value;
};

struct TransformComponent final
{
    Vec3 position{};
    Vec3 rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    Mat4 worldMatrix = Mat4::identity();
};

struct MeshRendererComponent final
{
    const Mesh* mesh = nullptr;
    bool castsShadows = true;
    bool visible = true;
    unsigned int visibilityMask = 0xffffffffu;
};

using RenderMeshComponent = MeshRendererComponent;

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

struct LightComponent final
{
    LocalLight light{};
    LightType type = LightType::Point;
    float attenuation = 1.0f;
    bool castsShadows = false;
    float volumetricScattering = 1.0f;
};

using LocalLightComponent = LightComponent;

struct VelocityComponent final
{
    Vec3 value{};
};

struct PlayerInputComponent final
{
    Vec2 moveAxes{};
    Vec2 lookDelta{};
    bool jumpPressed = false;
    bool jumpHeld = false;
    bool crouchHeld = false;
    bool sprintHeld = false;
    bool cursorCaptured = false;
};

struct ColliderComponent final
{
    float radius = 0.42f;
    float height = 1.8f;
    ColliderType type = ColliderType::Capsule;
    Vec3 halfExtents{0.42f, 0.9f, 0.42f};
    bool blocksMovement = true;
    bool allowsGrounding = true;
};

struct TerrainComponent final
{
    bool walkable = true;
};

struct PlayerComponent final
{
    bool active = true;
};

enum class PlayerTraversalState
{
    Grounded,
    Airborne,
    Crouching,
};

struct GroundingComponent final
{
    bool grounded = true;
    bool supportHit = false;
    bool supportRetained = false;
    Vec3 supportNormal{0.0f, 1.0f, 0.0f};
    Vec3 supportPoint{};
    float supportHeight = 0.0f;
    float supportDistance = 0.0f;
    float slopeAngleDegrees = 0.0f;
    float groundedDuration = 0.0f;
    float coyoteTimeRemaining = 0.0f;
    float supportPersistenceRemaining = 0.0f;
    int supportAcquisitionCount = 0;
    int groundedFrames = 0;
    int groundedTransitionCount = 0;
    int airborneTransitionCount = 0;
    int penetrationRecoveries = 0;
    int collisionCount = 0;
    int sweepIterations = 0;
};

struct CameraPresentationComponent final
{
    float standingEyeHeight = 1.64f;
    float crouchedEyeHeight = 1.02f;
    float currentEyeHeight = 1.64f;
    float bobPhase = 0.0f;
    float breathingPhase = 0.0f;
    float landingDip = 0.0f;
    float pitchOffsetDegrees = 0.0f;
    float rollDegrees = 0.0f;
    bool initialized = false;
    Vec3 previousBodyPosition{};
    Vec3 currentBodyPosition{};
    Vec3 localOffset{};
};

struct PlayerControllerComponent final
{
    PlayerTraversalState traversalState = PlayerTraversalState::Grounded;
    bool wantsCrouch = false;
    bool crouching = false;
    bool jumpRequested = false;
    float walkSpeed = 8.538f;
    float sprintMultiplier = 1.45f;
    float crouchSpeedMultiplier = 0.501f;
    float groundAcceleration = 42.0f;
    float airAcceleration = 10.0f;
    float groundFriction = 19.616f;
    float stopSpeed = 11.59f;
    float airControl = 0.308f;
    float jumpSpeed = 6.4f;
    float gravity = 17.365f;
    float maxSlopeAngleDegrees = 48.0f;
    float stepHeight = 0.55f;
    float supportProbeDistance = 0.18f;
    float sweepSkinWidth = 0.02f;
    float standingHeight = 1.8f;
    float crouchingHeight = 1.2f;
    float simulationHz = 120.0f;
    float coyoteTimeSeconds = 0.10f;
    float jumpBufferSeconds = 0.12f;
    float jumpBufferRemaining = 0.0f;
    int maxCollisionIterations = 6;
};

struct CameraComponent final
{
    float eyeHeight = 1.64f;
    bool active = false;
    bool debugFreeCamera = false;
    float yawDegrees = -90.0f;
    float pitchDegrees = 0.0f;
    float rollDegrees = 0.0f;
    float fieldOfViewRadians = 0.78539816339f;
    float nearPlane = 0.1f;
};
} // namespace engine::components
