#pragma once

#include "Application.h"
#include "ecs/Entity.h"
#include "world/WorldNavigation.h"

namespace engine
{
class Player;
class Scene;

class PlayerController final
{
  public:
    void update(Player& player, Scene& scene, ecs::Entity playerEntity,
                const InputState& inputState, float deltaSeconds);

    float walkSpeed() const noexcept;
    float sprintMultiplier() const noexcept;
    float jumpSpeed() const noexcept;
    float gravity() const noexcept;
    float mouseSensitivity() const noexcept;
    float groundAcceleration() const noexcept;
    float airAcceleration() const noexcept;
    float groundFriction() const noexcept;
    float airControl() const noexcept;
    float simulationHz() const noexcept;
    float crouchSpeedMultiplier() const noexcept;
    float stopSpeed() const noexcept;
    float capsuleRadius() const noexcept;
    float standingHeight() const noexcept;
    float crouchingHeight() const noexcept;
    float standingEyeHeight() const noexcept;
    float crouchedEyeHeight() const noexcept;
    float stepHeight() const noexcept;
    float supportProbeDistance() const noexcept;
    float maxSlopeAngleDegrees() const noexcept;
    float sweepSkinWidth() const noexcept;
    float coyoteTimeSeconds() const noexcept;
    float jumpBufferSeconds() const noexcept;
    int maxCollisionIterations() const noexcept;

    void setWalkSpeed(float walkSpeed) noexcept;
    void setSprintMultiplier(float sprintMultiplier) noexcept;
    void setJumpSpeed(float jumpSpeed) noexcept;
    void setGravity(float gravity) noexcept;
    void setMouseSensitivity(float mouseSensitivity) noexcept;
    void setGroundAcceleration(float acceleration) noexcept;
    void setAirAcceleration(float acceleration) noexcept;
    void setGroundFriction(float friction) noexcept;
    void setAirControl(float airControl) noexcept;
    void setSimulationHz(float simulationHz) noexcept;
    void setCrouchSpeedMultiplier(float crouchSpeedMultiplier) noexcept;
    void setStopSpeed(float stopSpeed) noexcept;
    void setCapsuleRadius(float capsuleRadius) noexcept;
    void setStandingHeight(float standingHeight) noexcept;
    void setCrouchingHeight(float crouchingHeight) noexcept;
    void setStandingEyeHeight(float standingEyeHeight) noexcept;
    void setCrouchedEyeHeight(float crouchedEyeHeight) noexcept;
    void setStepHeight(float stepHeight) noexcept;
    void setSupportProbeDistance(float supportProbeDistance) noexcept;
    void setMaxSlopeAngleDegrees(float maxSlopeAngleDegrees) noexcept;
    void setSweepSkinWidth(float sweepSkinWidth) noexcept;
    void setCoyoteTimeSeconds(float coyoteTimeSeconds) noexcept;
    void setJumpBufferSeconds(float jumpBufferSeconds) noexcept;
    void setMaxCollisionIterations(int maxCollisionIterations) noexcept;

  private:
    struct CollisionCache final
    {
        CollisionWorld world{};
        bool valid = false;
        std::size_t worldSignature = 0;
        int terrainSeed = 0;
        int terrainDensity = 0;
        float terrainScale = 0.0f;
        float terrainHeight = 0.0f;
    };

    void applyTuning(Scene& scene, ecs::Entity playerEntity) const;
    void writeInput(Scene& scene, ecs::Entity playerEntity, const InputState& inputState) const;
    void stepSimulation(Scene& scene, ecs::Entity playerEntity, float fixedDeltaSeconds,
                        MovementDebugState& debugState);
    void updatePresentation(Scene& scene, ecs::Entity playerEntity, float deltaSeconds,
                            MovementDebugState& debugState);
    void syncPlayerFromEcs(const Scene& scene, ecs::Entity playerEntity, Player& player,
                           float presentationAlpha) const;
    const CollisionWorld& collisionWorld(const Scene& scene);

    CollisionCache m_collisionCache{};
    bool m_collisionCacheRebuilt = false;
    bool m_staleColliderDetected = false;
    float m_accumulatorSeconds = 0.0f;
    float m_walkSpeed = 8.538f;
    float m_sprintMultiplier = 1.45f;
    float m_mouseSensitivity = 0.10f;
    float m_groundAcceleration = 42.0f;
    float m_airAcceleration = 10.0f;
    float m_groundFriction = 19.616f;
    float m_airControl = 0.308f;
    float m_jumpSpeed = 6.4f;
    float m_gravity = 17.365f;
    float m_simulationHz = 120.0f;
    float m_capsuleRadius = 0.42f;
    float m_standingHeight = 1.8f;
    float m_crouchingHeight = 1.2f;
    float m_standingEyeHeight = 1.64f;
    float m_crouchedEyeHeight = 1.02f;
    float m_stepHeight = 0.55f;
    float m_supportProbeDistance = 0.18f;
    float m_maxSlopeAngleDegrees = 48.0f;
    float m_sweepSkinWidth = 0.02f;
    float m_crouchSpeedMultiplier = 0.501f;
    float m_coyoteTimeSeconds = 0.10f;
    float m_jumpBufferSeconds = 0.12f;
    float m_stopSpeed = 11.59f;
    int m_maxCollisionIterations = 6;
};
} // namespace engine
