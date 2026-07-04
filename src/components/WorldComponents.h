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

/// @brief ECS component definitions for world objects, players, and cameras.
///
/// Each struct in this namespace represents a single piece of data that can be
/// attached to an `ecs::Entity` via the `ecs::Registry`. Components are plain
/// data (POD) with no behavior; systems read/write these components each frame.
namespace engine::components
{
/// @brief Type of light source for a LightComponent.
enum class LightType
{
    Directional,    ///< Infinite directional light (e.g. sun).
    Point,          ///< Omnidirectional point light.
    EmissiveVolume, ///< Volumetric emissive area.
};

/// @brief Collision shape approximation for a ColliderComponent.
enum class ColliderType
{
    Capsule,       ///< Capsule collider (default for characters).
    CylinderProxy, ///< Cylinder approximation.
    BoxProxy,      ///< Axis-aligned bounding box.
    TriangleMesh,  ///< Exact triangle mesh collision.
};

/// @brief Human-readable name attached to an entity.
struct NameComponent final
{
    std::string value; ///< Display or debug name for the entity.
};

/// @brief World-space transform data for an entity.
struct TransformComponent final
{
    Vec3 position{};                     ///< Translation in world space.
    Vec3 rotation{};                     ///< Euler rotation angles (degrees).
    Vec3 scale{1.0f, 1.0f, 1.0f};        ///< Uniform or non-uniform scale factors.
    Mat4 worldMatrix = Mat4::identity(); ///< Cached model matrix (position * rotation * scale).
};

/// @brief Rendering data for a mesh-backed entity.
struct MeshRendererComponent final
{
    const Mesh* mesh = nullptr;                ///< Pointer to the GPU-ready Mesh object.
    bool castsShadows = true;                  ///< Whether this mesh contributes to shadow maps.
    bool visible = true;                       ///< Whether this mesh is rendered.
    unsigned int visibilityMask = 0xffffffffu; ///< Bitmask for layer-based visibility culling.
};

/// @brief Legacy alias for MeshRendererComponent.
using RenderMeshComponent = MeshRendererComponent;

/// @brief Surface material properties for an entity.
struct MaterialComponent final
{
    Material material{}; ///< Albedo, roughness, metallic, emissive, etc.
};

/// @brief Semantic identification for world objects (interactive props, scenery, etc.).
struct WorldObjectComponent final
{
    WorldObjectId id = WorldObjectId::None;           ///< Unique world object identifier.
    WorldObjectKind kind = WorldObjectKind::Monolith; ///< Category of the object.
    unsigned int semantics =
        toSemanticFlags(WorldObjectSemantic::None); ///< Semantic flags bitmask.
    bool castsShadows = true;                       ///< Whether this object casts shadows.
};

/// @brief Marks an entity as interactable by the player.
struct InteractableComponent final
{
    std::string interactionPrompt = "Interact [E]"; ///< UI prompt text shown to the player.
    std::string interactionId;                      ///< Unique ID for interaction routing.
    float interactionRadius = 12.0f; ///< Maximum distance for interaction (world units).
    bool enabled = true;             ///< Whether interactions are currently active.
};

/// @brief Defines a spherical occlusion volume for ray-based queries.
struct RayOccluderComponent final
{
    Vec3 centerOffset{}; ///< Offset from entity origin to the occluder sphere center.
    float radius = 0.0f; ///< Radius of the occlusion sphere (world units).
};

/// @brief Attaches a light source to an entity.
struct LightComponent final
{
    LocalLight light{};                ///< Light color, intensity, and range data.
    LightType type = LightType::Point; ///< Classification of the light source.
    float attenuation = 1.0f;          ///< Distance attenuation multiplier.
    bool castsShadows = false;         ///< Whether this light generates shadow maps.
    float volumetricScattering = 1.0f; ///< Contribution to volumetric light shafts.
};

/// @brief Legacy alias for LightComponent.
using LocalLightComponent = LightComponent;

/// @brief Linear velocity for physics simulation.
struct VelocityComponent final
{
    Vec3 value{}; ///< Velocity vector in world units per second.
};

/// @brief Raw player input state captured each frame.
struct PlayerInputComponent final
{
    Vec2 moveAxes{};             ///< Movement input (forward/back, left/right).
    Vec2 lookDelta{};            ///< Camera rotation delta (yaw, pitch).
    bool jumpPressed = false;    ///< Jump button pressed this frame.
    bool jumpHeld = false;       ///< Jump button currently held.
    bool crouchHeld = false;     ///< Crouch button currently held.
    bool sprintHeld = false;     ///< Sprint button currently held.
    bool cursorCaptured = false; ///< Whether the mouse cursor is captured (FPS mode).
};

/// @brief Collision shape data for character or object physics.
struct ColliderComponent final
{
    float radius = 0.42f;                      ///< Capsule/cylinder radius (world units).
    float height = 1.8f;                       ///< Capsule/cylinder height (world units).
    ColliderType type = ColliderType::Capsule; ///< Shape approximation method.
    Vec3 halfExtents{0.42f, 0.9f, 0.42f};      ///< AABB half-extents for box proxies.
    bool blocksMovement = true;                ///< Whether this collider blocks player movement.
    bool allowsGrounding = true;               ///< Whether the player can stand on this surface.
};

/// @brief Marks a surface as terrain with walkability data.
struct TerrainComponent final
{
    bool walkable = true; ///< Whether the player can traverse this terrain.
};

/// @brief Marks an entity as the active player character.
struct PlayerComponent final
{
    bool active = true; ///< Whether this player entity is currently controlled.
};

/// @brief Current movement state of the player character.
enum class PlayerTraversalState
{
    Grounded,  ///< Standing or walking on a surface.
    Airborne,  ///< Jumping or falling.
    Crouching, ///< Crouched on the ground.
};

/// @brief Ground contact and slope data for the player character.
struct GroundingComponent final
{
    bool grounded = true;                     ///< Whether the player is currently on the ground.
    bool supportHit = false;                  ///< Whether a ground probe hit this frame.
    bool supportRetained = false;             ///< Whether previous ground contact is retained.
    Vec3 supportNormal{0.0f, 1.0f, 0.0f};     ///< Normal of the supporting surface.
    Vec3 supportPoint{};                      ///< World position of the ground contact point.
    float supportHeight = 0.0f;               ///< Height of the supporting surface.
    float supportDistance = 0.0f;             ///< Distance from character base to ground.
    float slopeAngleDegrees = 0.0f;           ///< Slope angle of the supporting surface.
    float groundedDuration = 0.0f;            ///< Time spent continuously grounded (seconds).
    float coyoteTimeRemaining = 0.0f;         ///< Remaining coyote time after leaving a ledge.
    float supportPersistenceRemaining = 0.0f; ///< Remaining ground retention time.
    int supportAcquisitionCount = 0;          ///< Number of times ground was acquired.
    int groundedFrames = 0;                   ///< Frame count while grounded.
    int groundedTransitionCount = 0;          ///< Number of airborne->grounded transitions.
    int airborneTransitionCount = 0;          ///< Number of grounded->airborne transitions.
    int penetrationRecoveries = 0;            ///< Number of collision penetration corrections.
    int collisionCount = 0;                   ///< Total collision events this frame.
    int sweepIterations = 0;                  ///< Sweep test iterations performed.
};

/// @brief Camera presentation parameters (head bob, breathing, landing dip).
struct CameraPresentationComponent final
{
    float standingEyeHeight = 1.64f;    ///< Eye height when standing (meters).
    float crouchedEyeHeight = 1.02f;    ///< Eye height when crouching (meters).
    float currentEyeHeight = 1.64f;     ///< Current interpolated eye height.
    float bobBlend = 0.0f;              ///< Head bob intensity blend factor.
    float bobPhase = 0.0f;              ///< Current head bob cycle phase.
    float breathingPhase = 0.0f;        ///< Breathing animation phase.
    float landingDip = 0.0f;            ///< Camera dip amount after landing.
    float pitchOffsetDegrees = 0.0f;    ///< Additional pitch offset (degrees).
    float rollDegrees = 0.0f;           ///< Camera roll angle (degrees).
    float bobPitchOffsetDegrees = 0.0f; ///< Pitch contribution from head bob.
    float bobRollDegrees = 0.0f;        ///< Roll contribution from head bob.
    bool initialized = false;           ///< Whether presentation state is initialized.
    Vec3 previousBodyPosition{};        ///< Body position last frame.
    Vec3 currentBodyPosition{};         ///< Body position this frame.
    Vec3 localOffset{};                 ///< Local camera offset from body origin.
};

/// @brief Player movement controller configuration and state.
struct PlayerControllerComponent final
{
    PlayerTraversalState traversalState =
        PlayerTraversalState::Grounded;   ///< Current movement state.
    bool wantsCrouch = false;             ///< Player input: wants to crouch.
    bool crouching = false;               ///< Currently in crouch state.
    bool jumpRequested = false;           ///< Jump was requested this frame.
    float walkSpeed = 8.538f;             ///< Base walking speed (units/s).
    float sprintMultiplier = 1.45f;       ///< Speed multiplier while sprinting.
    float crouchSpeedMultiplier = 0.501f; ///< Speed multiplier while crouching.
    float groundAcceleration = 42.0f;     ///< Horizontal acceleration on ground (units/s^2).
    float airAcceleration = 10.0f;        ///< Horizontal acceleration in air (units/s^2).
    float groundFriction = 19.616f;       ///< Deceleration when no input on ground (units/s^2).
    float stopSpeed = 11.59f;             ///< Speed threshold below which friction is stronger.
    float airControl = 0.308f;            ///< Airborne input responsiveness multiplier.
    float jumpSpeed = 6.4f;               ///< Initial vertical velocity on jump (units/s).
    float gravity = 17.365f;              ///< Downward acceleration (units/s^2).
    float maxSlopeAngleDegrees = 48.0f;   ///< Maximum walkable slope angle.
    float stepHeight = 0.55f;             ///< Maximum step height the player can climb.
    float supportProbeDistance = 0.18f;   ///< Distance for ground support raycast.
    float sweepSkinWidth = 0.02f;         ///< Extra radius for collision sweep tests.
    float standingHeight = 1.8f;          ///< Player height when standing (meters).
    float crouchingHeight = 1.2f;         ///< Player height when crouching (meters).
    float simulationHz = 120.0f;          ///< Physics simulation update rate (Hz).
    float coyoteTimeSeconds = 0.10f;      ///< Grace period for jumping after leaving a ledge.
    float jumpBufferSeconds = 0.12f;      ///< Grace period for jump input before landing.
    float jumpBufferRemaining = 0.0f;     ///< Remaining buffered jump time (seconds).
    int maxCollisionIterations = 6;       ///< Maximum collision resolution steps per frame.
};

/// @brief Camera component with orientation and projection settings.
struct CameraComponent final
{
    float eyeHeight = 1.64f;                   ///< Camera height above entity origin (meters).
    bool active = false;                       ///< Whether this camera is currently active.
    bool debugFreeCamera = false;              ///< Whether free-camera debug mode is enabled.
    float yawDegrees = -90.0f;                 ///< Horizontal rotation angle (degrees).
    float pitchDegrees = 0.0f;                 ///< Vertical rotation angle (degrees).
    float rollDegrees = 0.0f;                  ///< Camera roll angle (degrees).
    float fieldOfViewRadians = 0.78539816339f; ///< Vertical FOV in radians (default 45 deg).
    float nearPlane = 0.1f;                    ///< Near clipping plane distance.
};
} // namespace engine::components
