#include "world/PlayerController.h"

#include "world/Player.h"
#include "world/Scene.h"

#include <algorithm>

namespace
{
engine::Vec3 flattenOrFallback(const engine::Vec3& value, const engine::Vec3& fallback)
{
    const engine::Vec3 flattened{value.x, 0.0f, value.z};
    if (engine::length(flattened) <= 0.0001f)
    {
        return fallback;
    }

    return engine::normalize(flattened);
}

float approachScalar(float current, float target, float maximumDelta)
{
    if (current < target)
    {
        return std::min(current + maximumDelta, target);
    }

    return std::max(current - maximumDelta, target);
}

engine::Vec3 approachVector(const engine::Vec3& current, const engine::Vec3& target,
                            float maximumDelta)
{
    return engine::Vec3{
        approachScalar(current.x, target.x, maximumDelta),
        approachScalar(current.y, target.y, maximumDelta),
        approachScalar(current.z, target.z, maximumDelta),
    };
}
} // namespace

namespace engine
{
void PlayerController::update(Player& player, const Scene& scene, const InputState& inputState,
                              float deltaSeconds) const
{
    if (inputState.cursorCaptured)
    {
        player.rotate(inputState.mouseDelta.x * m_mouseSensitivity,
                      inputState.mouseDelta.y * m_mouseSensitivity);
    }

    if (deltaSeconds <= 0.0f)
    {
        player.syncCamera();
        return;
    }

    Vec3 velocity = player.velocity();
    Vec3 desiredDirection{};
    bool jumpedThisFrame = false;
    const Vec3 forward = flattenOrFallback(player.camera().front(), Vec3{0.0f, 0.0f, -1.0f});
    const Vec3 right = flattenOrFallback(player.camera().right(), Vec3{1.0f, 0.0f, 0.0f});

    if (inputState.cursorCaptured)
    {
        if (inputState.moveForward)
        {
            desiredDirection = desiredDirection + forward;
        }

        if (inputState.moveBackward)
        {
            desiredDirection = desiredDirection - forward;
        }

        if (inputState.moveRight)
        {
            desiredDirection = desiredDirection + right;
        }

        if (inputState.moveLeft)
        {
            desiredDirection = desiredDirection - right;
        }
    }

    const float targetSpeed =
        m_walkSpeed * (inputState.sprint && player.grounded() ? m_sprintMultiplier : 1.0f);
    const Vec3 targetHorizontalVelocity =
        length(desiredDirection) > 0.0001f ? normalize(desiredDirection) * targetSpeed : Vec3{};
    const Vec3 currentHorizontalVelocity{velocity.x, 0.0f, velocity.z};
    const float acceleration = player.grounded() ? m_groundAcceleration : m_airAcceleration;
    const Vec3 nextHorizontalVelocity = approachVector(
        currentHorizontalVelocity, targetHorizontalVelocity, acceleration * deltaSeconds);
    velocity.x = nextHorizontalVelocity.x;
    velocity.z = nextHorizontalVelocity.z;

    if (player.grounded())
    {
        velocity.y = std::min(velocity.y, 0.0f);
        if (inputState.jump)
        {
            velocity.y = m_jumpSpeed;
            player.setGrounded(false);
            jumpedThisFrame = true;
        }
    }

    if (!player.grounded())
    {
        velocity.y -= m_gravity * deltaSeconds;
    }

    const Vec3 previousPosition = player.position();
    Vec3 candidatePosition = previousPosition;
    candidatePosition.x += velocity.x * deltaSeconds;
    candidatePosition.z += velocity.z * deltaSeconds;

    if (player.grounded())
    {
        const float previousGroundHeight =
            sampleAtmosphericTerrainHeight(scene, previousPosition.x, previousPosition.z);
        const float candidateGroundHeight =
            sampleAtmosphericTerrainHeight(scene, candidatePosition.x, candidatePosition.z);
        const Vec3 candidateGroundNormal =
            sampleAtmosphericTerrainNormal(scene, candidatePosition.x, candidatePosition.z);

        if ((candidateGroundHeight - previousGroundHeight) > m_maxStepHeight ||
            candidateGroundNormal.y < m_minGroundNormalY)
        {
            candidatePosition.x = previousPosition.x;
            candidatePosition.z = previousPosition.z;
            velocity.x = 0.0f;
            velocity.z = 0.0f;
        }
    }

    candidatePosition.y += velocity.y * deltaSeconds;
    if (player.grounded() && velocity.y <= 0.0f)
    {
        candidatePosition.y -= m_groundSnapDistance;
    }

    resolveAtmosphericWorldCollision(
        scene, previousPosition, candidatePosition,
        CylinderCollisionShape{player.collisionRadius(), player.collisionHeight()},
        m_maxStepHeight);

    float groundHeight =
        sampleAtmosphericTerrainHeight(scene, candidatePosition.x, candidatePosition.z);
    const SupportHeightResult objectSupport = sampleAtmosphericObjectSupportHeight(
        scene, candidatePosition.x, candidatePosition.z,
        std::min(previousPosition.y, candidatePosition.y) - m_groundSnapDistance,
        std::max(previousPosition.y, candidatePosition.y) + m_maxStepHeight);
    if (objectSupport.hit)
    {
        groundHeight = std::max(groundHeight, objectSupport.height);
    }

    if (!jumpedThisFrame && candidatePosition.y <= groundHeight + m_groundSnapDistance &&
        velocity.y <= 0.0f)
    {
        candidatePosition.y = groundHeight;
        velocity.y = 0.0f;
        player.setGrounded(true);
    }
    else
    {
        player.setGrounded(false);
    }

    player.setPosition(candidatePosition);
    player.setVelocity(velocity);
}

float PlayerController::walkSpeed() const noexcept
{
    return m_walkSpeed;
}

float PlayerController::sprintMultiplier() const noexcept
{
    return m_sprintMultiplier;
}

float PlayerController::jumpSpeed() const noexcept
{
    return m_jumpSpeed;
}

float PlayerController::gravity() const noexcept
{
    return m_gravity;
}

float PlayerController::mouseSensitivity() const noexcept
{
    return m_mouseSensitivity;
}

void PlayerController::setWalkSpeed(float walkSpeed) noexcept
{
    m_walkSpeed = std::max(walkSpeed, 0.1f);
}

void PlayerController::setSprintMultiplier(float sprintMultiplier) noexcept
{
    m_sprintMultiplier = std::max(sprintMultiplier, 1.0f);
}

void PlayerController::setJumpSpeed(float jumpSpeed) noexcept
{
    m_jumpSpeed = std::max(jumpSpeed, 0.0f);
}

void PlayerController::setGravity(float gravity) noexcept
{
    m_gravity = std::max(gravity, 0.1f);
}

void PlayerController::setMouseSensitivity(float mouseSensitivity) noexcept
{
    m_mouseSensitivity = std::max(mouseSensitivity, 0.001f);
}
} // namespace engine
