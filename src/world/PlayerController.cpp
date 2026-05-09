#include "world/PlayerController.h"

#include "components/WorldComponents.h"
#include "world/Player.h"
#include "world/Scene.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>

namespace
{
constexpr float kPi = 3.14159265359f;
constexpr float kMaxAccumulatedSeconds = 0.25f;
constexpr int kMaxFixedStepsPerFrame = 8;
constexpr float kGroundRetainDistance = 0.08f;
constexpr float kGroundRetainVerticalSpeed = 2.5f;
constexpr float kSupportPersistenceSeconds = 0.08f;
constexpr float kMaxEffectiveBobStrength = 0.85f;
constexpr float kMaxLandingDip = 0.028f;
constexpr float kBreathingAmplitude = 0.0022f;
constexpr float kBobSideAmplitude = 0.0011f;
constexpr float kBobUpAmplitude = 0.0020f;
constexpr float kBobPitchAmplitudeDegrees = 0.035f;
constexpr float kBobRollAmplitudeDegrees = 0.045f;
constexpr float kMovePitchAmplitudeDegrees = 0.12f;
constexpr float kMoveRollAmplitudeDegrees = 0.35f;

float radians(float degrees)
{
    return degrees * kPi / 180.0f;
}

float approachScalar(float current, float target, float maximumDelta)
{
    if (current < target)
    {
        return std::min(current + maximumDelta, target);
    }

    return std::max(current - maximumDelta, target);
}

engine::Vec3 horizontalVector(const engine::Vec3& value)
{
    return engine::Vec3{value.x, 0.0f, value.z};
}

engine::Vec3 projectOntoPlane(const engine::Vec3& value, const engine::Vec3& normal)
{
    return value - normal * engine::dot(value, normal);
}

engine::Vec3 groundMovementVector(const engine::Vec3& intent, const engine::Vec3& supportNormal,
                                  float speed)
{
    const engine::Vec3 projectedDirection = projectOntoPlane(intent, supportNormal);
    const float projectedLength = engine::length(projectedDirection);
    if (projectedLength <= 1.0e-4f)
    {
        return engine::Vec3{};
    }

    const engine::Vec3 normalizedDirection = projectedDirection / projectedLength;
    const float uphillFactor = std::clamp(normalizedDirection.y, 0.0f, 1.0f);
    const float uphillResistance = 1.0f - uphillFactor * uphillFactor * 0.2f;
    return normalizedDirection * (speed * uphillResistance);
}

engine::Vec3 lerpVec3(const engine::Vec3& start, const engine::Vec3& end, float alpha)
{
    return start + (end - start) * alpha;
}

template <typename ValueType> void hashCombine(std::size_t& seed, const ValueType& value)
{
    seed ^= std::hash<ValueType>{}(value) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
}

bool isMovementSolidObject(const engine::components::WorldObjectComponent& object)
{
    if ((object.semantics & engine::toSemanticFlags(engine::WorldObjectSemantic::Surface)) == 0u)
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

std::size_t collisionWorldSignature(const engine::Scene& scene)
{
    std::size_t signature = 0;
    scene.registry()
        .forEach<engine::components::WorldObjectComponent, engine::components::TransformComponent,
                 engine::components::RenderMeshComponent>(
            [&](engine::ecs::Entity entity, const engine::components::WorldObjectComponent& object,
                const engine::components::TransformComponent& transform,
                const engine::components::RenderMeshComponent& renderMesh)
            {
                if (!isMovementSolidObject(object) || renderMesh.mesh == nullptr)
                {
                    return;
                }

                hashCombine(signature, static_cast<std::uint32_t>(entity));
                hashCombine(signature, static_cast<std::uint32_t>(object.id));
                hashCombine(signature, static_cast<std::uint32_t>(object.kind));
                hashCombine(signature, object.semantics);
                hashCombine(signature, reinterpret_cast<std::uintptr_t>(
                                           static_cast<const void*>(renderMesh.mesh)));
                hashCombine(signature, transform.position.x);
                hashCombine(signature, transform.position.y);
                hashCombine(signature, transform.position.z);
                hashCombine(signature, transform.rotation.x);
                hashCombine(signature, transform.rotation.y);
                hashCombine(signature, transform.rotation.z);
                hashCombine(signature, transform.scale.x);
                hashCombine(signature, transform.scale.y);
                hashCombine(signature, transform.scale.z);
            });
    return signature;
}

engine::Vec3 applyFriction(const engine::Vec3& velocity, float friction, float stopSpeed,
                           float deltaSeconds)
{
    const float speed = engine::length(velocity);
    if (speed <= 0.0001f)
    {
        return engine::Vec3{};
    }

    const float control = std::max(speed, stopSpeed);
    const float drop = control * friction * deltaSeconds;
    const float nextSpeed = std::max(speed - drop, 0.0f);
    if (nextSpeed <= 0.0001f)
    {
        return engine::Vec3{};
    }

    return velocity * (nextSpeed / speed);
}

engine::Vec3 applyGroundFrictionTowardDesired(const engine::Vec3& velocity,
                                              const engine::Vec3& desiredVelocity, float friction,
                                              float stopSpeed, float deltaSeconds)
{
    const float desiredSpeed = engine::length(desiredVelocity);
    if (desiredSpeed <= 0.0001f)
    {
        return applyFriction(velocity, friction, stopSpeed, deltaSeconds);
    }

    const engine::Vec3 desiredDirection = desiredVelocity / desiredSpeed;
    const float alongSpeed = engine::dot(velocity, desiredDirection);
    const engine::Vec3 alongVelocity = desiredDirection * alongSpeed;
    const engine::Vec3 lateralVelocity = velocity - alongVelocity;

    engine::Vec3 adjustedAlongVelocity = alongVelocity;
    if (alongSpeed < 0.0f)
    {
        adjustedAlongVelocity = applyFriction(alongVelocity, friction, stopSpeed, deltaSeconds);
    }
    else if (alongSpeed > desiredSpeed)
    {
        adjustedAlongVelocity = applyFriction(alongVelocity, friction, desiredSpeed, deltaSeconds);
    }

    const engine::Vec3 adjustedLateralVelocity =
        applyFriction(lateralVelocity, friction, 0.0f, deltaSeconds);

    return adjustedAlongVelocity + adjustedLateralVelocity;
}

engine::Vec3 accelerateToward(const engine::Vec3& velocity, const engine::Vec3& targetVelocity,
                              float acceleration, float deltaSeconds)
{
    const engine::Vec3 delta = targetVelocity - velocity;
    const float deltaLength = engine::length(delta);
    if (deltaLength <= 0.0001f)
    {
        return targetVelocity;
    }

    const float maxDelta = acceleration * deltaSeconds;
    if (deltaLength <= maxDelta)
    {
        return targetVelocity;
    }

    return velocity + delta * (maxDelta / deltaLength);
}

void yawBasis(float yawDegrees, engine::Vec3& forward, engine::Vec3& right)
{
    const float yawRadians = radians(yawDegrees);
    forward = engine::normalize(engine::Vec3{std::cos(yawRadians), 0.0f, std::sin(yawRadians)});
    right = engine::normalize(engine::Vec3{-forward.z, 0.0f, forward.x});
}
} // namespace

namespace engine
{
void PlayerController::update(Player& player, Scene& scene,
                              const ProceduralWorldSettings& worldSettings,
                              MovementDebugState& movementDebugState, ecs::Entity playerEntity,
                              const ExplorationInputState& inputState, float deltaSeconds)
{
    if (!scene.registry().isAlive(playerEntity))
    {
        return;
    }

    applyTuning(scene, playerEntity);
    writeInput(scene, playerEntity, inputState);

    MovementDebugState debugState{};
    debugState.deltaSeconds = deltaSeconds;
    debugState.cursorCaptured = inputState.cursorCaptured;

    const components::PlayerControllerComponent& controller =
        scene.registry().get<components::PlayerControllerComponent>(playerEntity);
    const float fixedDeltaSeconds = 1.0f / std::max(controller.simulationHz, 30.0f);

    const float nextAccumulator = m_accumulatorSeconds + std::max(deltaSeconds, 0.0f);
    debugState.droppedSimulationSeconds = std::max(nextAccumulator - kMaxAccumulatedSeconds, 0.0f);
    m_accumulatorSeconds = std::min(nextAccumulator, kMaxAccumulatedSeconds);

    int fixedSteps = 0;
    while (m_accumulatorSeconds >= fixedDeltaSeconds && fixedSteps < kMaxFixedStepsPerFrame)
    {
        stepSimulation(scene, worldSettings, playerEntity, fixedDeltaSeconds, debugState);
        m_accumulatorSeconds -= fixedDeltaSeconds;
        ++fixedSteps;
    }

    debugState.fixedSteps = fixedSteps;
    debugState.simulationStepSeconds = fixedDeltaSeconds;
    debugState.accumulatorSeconds = m_accumulatorSeconds;
    debugState.simulationClamped =
        fixedSteps == kMaxFixedStepsPerFrame && m_accumulatorSeconds >= fixedDeltaSeconds;
    debugState.presentationAlpha =
        fixedDeltaSeconds > 0.0f ? std::clamp(m_accumulatorSeconds / fixedDeltaSeconds, 0.0f, 1.0f)
                                 : 0.0f;

    updatePresentation(scene, playerEntity, deltaSeconds, debugState);
    syncPlayerFromEcs(scene, playerEntity, player, debugState.presentationAlpha);
    movementDebugState = debugState;
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

float PlayerController::cameraBobAmount() const noexcept
{
    return m_cameraBobAmount;
}

float PlayerController::cameraBobBlendSpeed() const noexcept
{
    return m_cameraBobBlendSpeed;
}

float PlayerController::groundAcceleration() const noexcept
{
    return m_groundAcceleration;
}

float PlayerController::airAcceleration() const noexcept
{
    return m_airAcceleration;
}

float PlayerController::groundFriction() const noexcept
{
    return m_groundFriction;
}

float PlayerController::airControl() const noexcept
{
    return m_airControl;
}

float PlayerController::simulationHz() const noexcept
{
    return m_simulationHz;
}

float PlayerController::crouchSpeedMultiplier() const noexcept
{
    return m_crouchSpeedMultiplier;
}

float PlayerController::stopSpeed() const noexcept
{
    return m_stopSpeed;
}

float PlayerController::capsuleRadius() const noexcept
{
    return m_capsuleRadius;
}

float PlayerController::standingHeight() const noexcept
{
    return m_standingHeight;
}

float PlayerController::crouchingHeight() const noexcept
{
    return m_crouchingHeight;
}

float PlayerController::standingEyeHeight() const noexcept
{
    return m_standingEyeHeight;
}

float PlayerController::crouchedEyeHeight() const noexcept
{
    return m_crouchedEyeHeight;
}

float PlayerController::stepHeight() const noexcept
{
    return m_stepHeight;
}

float PlayerController::supportProbeDistance() const noexcept
{
    return m_supportProbeDistance;
}

float PlayerController::maxSlopeAngleDegrees() const noexcept
{
    return m_maxSlopeAngleDegrees;
}

float PlayerController::sweepSkinWidth() const noexcept
{
    return m_sweepSkinWidth;
}

float PlayerController::coyoteTimeSeconds() const noexcept
{
    return m_coyoteTimeSeconds;
}

float PlayerController::jumpBufferSeconds() const noexcept
{
    return m_jumpBufferSeconds;
}

int PlayerController::maxCollisionIterations() const noexcept
{
    return m_maxCollisionIterations;
}

void PlayerController::setWalkSpeed(float walkSpeed) noexcept
{
    m_walkSpeed = std::max(walkSpeed, 0.5f);
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
    m_mouseSensitivity = std::clamp(mouseSensitivity, 0.01f, 1.0f);
}

void PlayerController::setCameraBobAmount(float cameraBobAmount) noexcept
{
    m_cameraBobAmount = std::clamp(cameraBobAmount, 0.0f, 3.0f);
}

void PlayerController::setCameraBobBlendSpeed(float cameraBobBlendSpeed) noexcept
{
    m_cameraBobBlendSpeed = std::clamp(cameraBobBlendSpeed, 0.5f, 12.0f);
}

void PlayerController::setGroundAcceleration(float acceleration) noexcept
{
    m_groundAcceleration = std::max(acceleration, 0.1f);
}

void PlayerController::setAirAcceleration(float acceleration) noexcept
{
    m_airAcceleration = std::max(acceleration, 0.1f);
}

void PlayerController::setGroundFriction(float friction) noexcept
{
    m_groundFriction = std::max(friction, 0.0f);
}

void PlayerController::setAirControl(float airControl) noexcept
{
    m_airControl = std::clamp(airControl, 0.0f, 1.0f);
}

void PlayerController::setSimulationHz(float simulationHz) noexcept
{
    m_simulationHz = std::clamp(simulationHz, 30.0f, 240.0f);
}

void PlayerController::setCrouchSpeedMultiplier(float crouchSpeedMultiplier) noexcept
{
    m_crouchSpeedMultiplier = std::clamp(crouchSpeedMultiplier, 0.1f, 1.0f);
}

void PlayerController::setStopSpeed(float stopSpeed) noexcept
{
    m_stopSpeed = std::max(stopSpeed, 0.1f);
}

void PlayerController::setCapsuleRadius(float capsuleRadius) noexcept
{
    m_capsuleRadius = std::clamp(capsuleRadius, 0.1f, 1.0f);
}

void PlayerController::setStandingHeight(float standingHeight) noexcept
{
    m_standingHeight = std::max(standingHeight, m_capsuleRadius * 2.0f + 0.1f);
    m_standingEyeHeight = std::min(m_standingEyeHeight, m_standingHeight - 0.05f);
    m_crouchingHeight = std::min(m_crouchingHeight, m_standingHeight);
}

void PlayerController::setCrouchingHeight(float crouchingHeight) noexcept
{
    const float minimumHeight = m_capsuleRadius * 2.0f + 0.1f;
    m_crouchingHeight = std::clamp(crouchingHeight, minimumHeight, m_standingHeight);
    m_crouchedEyeHeight = std::min(m_crouchedEyeHeight, m_crouchingHeight - 0.05f);
}

void PlayerController::setStandingEyeHeight(float standingEyeHeight) noexcept
{
    m_standingEyeHeight = std::clamp(standingEyeHeight, 0.1f, m_standingHeight - 0.05f);
}

void PlayerController::setCrouchedEyeHeight(float crouchedEyeHeight) noexcept
{
    m_crouchedEyeHeight = std::clamp(crouchedEyeHeight, 0.1f, m_crouchingHeight - 0.05f);
}

void PlayerController::setStepHeight(float stepHeight) noexcept
{
    m_stepHeight = std::clamp(stepHeight, 0.0f, 1.2f);
}

void PlayerController::setSupportProbeDistance(float supportProbeDistance) noexcept
{
    m_supportProbeDistance = std::clamp(supportProbeDistance, 0.01f, 0.5f);
}

void PlayerController::setMaxSlopeAngleDegrees(float maxSlopeAngleDegrees) noexcept
{
    m_maxSlopeAngleDegrees = std::clamp(maxSlopeAngleDegrees, 0.0f, 89.0f);
}

void PlayerController::setSweepSkinWidth(float sweepSkinWidth) noexcept
{
    m_sweepSkinWidth = std::clamp(sweepSkinWidth, 0.001f, 0.1f);
}

void PlayerController::setCoyoteTimeSeconds(float coyoteTimeSeconds) noexcept
{
    m_coyoteTimeSeconds = std::clamp(coyoteTimeSeconds, 0.0f, 0.5f);
}

void PlayerController::setJumpBufferSeconds(float jumpBufferSeconds) noexcept
{
    m_jumpBufferSeconds = std::clamp(jumpBufferSeconds, 0.0f, 0.5f);
}

void PlayerController::setMaxCollisionIterations(int maxCollisionIterations) noexcept
{
    m_maxCollisionIterations = std::clamp(maxCollisionIterations, 1, 12);
}

void PlayerController::applyTuning(Scene& scene, ecs::Entity playerEntity) const
{
    ecs::Registry& registry = scene.registry();
    components::PlayerControllerComponent& controller =
        registry.get<components::PlayerControllerComponent>(playerEntity);
    controller.walkSpeed = m_walkSpeed;
    controller.sprintMultiplier = m_sprintMultiplier;
    controller.crouchSpeedMultiplier = m_crouchSpeedMultiplier;
    controller.groundAcceleration = m_groundAcceleration;
    controller.airAcceleration = m_airAcceleration;
    controller.groundFriction = m_groundFriction;
    controller.stopSpeed = m_stopSpeed;
    controller.airControl = m_airControl;
    controller.jumpSpeed = m_jumpSpeed;
    controller.gravity = m_gravity;
    controller.maxSlopeAngleDegrees = m_maxSlopeAngleDegrees;
    controller.stepHeight = m_stepHeight;
    controller.supportProbeDistance = m_supportProbeDistance;
    controller.sweepSkinWidth = m_sweepSkinWidth;
    controller.standingHeight = m_standingHeight;
    controller.crouchingHeight = m_crouchingHeight;
    controller.simulationHz = m_simulationHz;
    controller.coyoteTimeSeconds = m_coyoteTimeSeconds;
    controller.jumpBufferSeconds = m_jumpBufferSeconds;
    controller.maxCollisionIterations = m_maxCollisionIterations;

    components::ColliderComponent& collider =
        registry.get<components::ColliderComponent>(playerEntity);
    collider.radius = m_capsuleRadius;
    collider.type = components::ColliderType::Capsule;
    collider.height = controller.crouching ? m_crouchingHeight : m_standingHeight;
    collider.halfExtents = Vec3{m_capsuleRadius, collider.height * 0.5f, m_capsuleRadius};

    components::CameraPresentationComponent& presentation =
        registry.get<components::CameraPresentationComponent>(playerEntity);
    presentation.standingEyeHeight = m_standingEyeHeight;
    presentation.crouchedEyeHeight = m_crouchedEyeHeight;
}

void PlayerController::writeInput(Scene& scene, ecs::Entity playerEntity,
                                  const ExplorationInputState& inputState) const
{
    ecs::Registry& registry = scene.registry();
    components::PlayerInputComponent& input =
        registry.get<components::PlayerInputComponent>(playerEntity);
    input.moveAxes =
        Vec2{(inputState.moveRight ? 1.0f : 0.0f) - (inputState.moveLeft ? 1.0f : 0.0f),
             (inputState.moveForward ? 1.0f : 0.0f) - (inputState.moveBackward ? 1.0f : 0.0f)};
    if (length(input.moveAxes) > 1.0f)
    {
        input.moveAxes = normalize(input.moveAxes);
    }
    input.lookDelta = inputState.mouseDelta;
    input.jumpPressed = inputState.jump && !input.jumpHeld;
    input.jumpHeld = inputState.jump;
    input.crouchHeld = inputState.crouch;
    input.sprintHeld = inputState.sprint;
    input.cursorCaptured = inputState.cursorCaptured;

    components::CameraComponent& camera = registry.get<components::CameraComponent>(playerEntity);
    if (inputState.cursorCaptured)
    {
        camera.yawDegrees += inputState.mouseDelta.x * m_mouseSensitivity;
        camera.pitchDegrees = std::clamp(
            camera.pitchDegrees + inputState.mouseDelta.y * m_mouseSensitivity, -89.0f, 89.0f);
    }
}

void PlayerController::stepSimulation(Scene& scene, const ProceduralWorldSettings& worldSettings,
                                      ecs::Entity playerEntity, float fixedDeltaSeconds,
                                      MovementDebugState& debugState)
{
    ecs::Registry& registry = scene.registry();
    components::TransformComponent& transform =
        registry.get<components::TransformComponent>(playerEntity);
    components::VelocityComponent& velocity =
        registry.get<components::VelocityComponent>(playerEntity);
    components::ColliderComponent& collider =
        registry.get<components::ColliderComponent>(playerEntity);
    components::PlayerControllerComponent& controller =
        registry.get<components::PlayerControllerComponent>(playerEntity);
    components::PlayerInputComponent& input =
        registry.get<components::PlayerInputComponent>(playerEntity);
    components::GroundingComponent& grounding =
        registry.get<components::GroundingComponent>(playerEntity);
    components::CameraComponent& camera = registry.get<components::CameraComponent>(playerEntity);
    components::CameraPresentationComponent& presentation =
        registry.get<components::CameraPresentationComponent>(playerEntity);

    if (!presentation.initialized)
    {
        presentation.initialized = true;
        presentation.previousBodyPosition = transform.position;
        presentation.currentBodyPosition = transform.position;
    }

    const CollisionWorld& world = collisionWorld(scene, worldSettings);
    debugState.collisionCacheRebuilt = m_collisionCacheRebuilt;
    debugState.staleColliderDetected = m_staleColliderDetected;

    controller.wantsCrouch = input.crouchHeld;
    if (controller.wantsCrouch)
    {
        controller.crouching = true;
    }
    else if (controller.crouching)
    {
        if (collisionWorldCanOccupy(
                world, transform.position,
                CapsuleCollisionShape{collider.radius, controller.standingHeight},
                controller.sweepSkinWidth))
        {
            controller.crouching = false;
        }
    }

    collider.height = controller.crouching ? controller.crouchingHeight : controller.standingHeight;
    collider.halfExtents = Vec3{collider.radius, collider.height * 0.5f, collider.radius};

    if (input.jumpPressed)
    {
        controller.jumpBufferRemaining = controller.jumpBufferSeconds;
        input.jumpPressed = false;
    }
    else
    {
        controller.jumpBufferRemaining =
            std::max(controller.jumpBufferRemaining - fixedDeltaSeconds, 0.0f);
    }

    const Vec3 previousVelocity = velocity.value;
    const Vec3 startPosition = transform.position;
    const CapsuleCollisionShape shape{collider.radius, collider.height};
    const float supportRetainDistance = controller.supportProbeDistance + kGroundRetainDistance;
    const SupportInfo initialSupport =
        queryGroundSupport(world, transform.position, shape, controller.supportProbeDistance,
                           controller.maxSlopeAngleDegrees);
    const float initialSupportSeparationSpeed = initialSupport.walkable
                                                    ? dot(velocity.value, initialSupport.normal)
                                                    : dot(velocity.value, grounding.supportNormal);
    const bool directTouchingGround = initialSupport.walkable &&
                                      initialSupport.distance <= controller.supportProbeDistance &&
                                      initialSupportSeparationSpeed <= 1.0f;
    if (directTouchingGround)
    {
        grounding.supportPersistenceRemaining = kSupportPersistenceSeconds;
    }
    else
    {
        grounding.supportPersistenceRemaining =
            std::max(grounding.supportPersistenceRemaining - fixedDeltaSeconds, 0.0f);
    }

    const bool retainedTouchingGround =
        grounding.grounded && grounding.supportPersistenceRemaining > 0.0f &&
        initialSupport.walkable && initialSupport.distance <= supportRetainDistance &&
        initialSupportSeparationSpeed <= kGroundRetainVerticalSpeed;
    const bool touchingGround = directTouchingGround || retainedTouchingGround;
    grounding.supportRetained = retainedTouchingGround && !directTouchingGround;
    const Vec3 supportNormal =
        directTouchingGround ? initialSupport.normal : grounding.supportNormal;

    if (touchingGround)
    {
        grounding.coyoteTimeRemaining = controller.coyoteTimeSeconds;
    }
    else
    {
        grounding.coyoteTimeRemaining =
            std::max(grounding.coyoteTimeRemaining - fixedDeltaSeconds, 0.0f);
    }

    Vec3 forward{};
    Vec3 right{};
    yawBasis(camera.yawDegrees, forward, right);
    Vec3 intent = forward * input.moveAxes.y + right * input.moveAxes.x;
    const float intentMagnitude = length(intent);
    if (intentMagnitude > 1.0e-4f)
    {
        intent = intent / intentMagnitude;
    }

    const float speedMultiplier = controller.crouching
                                      ? controller.crouchSpeedMultiplier
                                      : (input.sprintHeld ? controller.sprintMultiplier : 1.0f);
    const float desiredSpeed = controller.walkSpeed * speedMultiplier;
    Vec3 desiredVelocity = intent * desiredSpeed;
    if (touchingGround)
    {
        desiredVelocity = groundMovementVector(intent, supportNormal, desiredSpeed);
    }

    if (touchingGround)
    {
        Vec3 tangentVelocity = projectOntoPlane(velocity.value, supportNormal);
        const Vec3 preFrictionVelocity = tangentVelocity;
        tangentVelocity = applyGroundFrictionTowardDesired(tangentVelocity, desiredVelocity,
                                                           controller.groundFriction,
                                                           controller.stopSpeed, fixedDeltaSeconds);
        const float frictionImpulse = length(preFrictionVelocity - tangentVelocity);
        tangentVelocity = accelerateToward(tangentVelocity, desiredVelocity,
                                           controller.groundAcceleration, fixedDeltaSeconds);
        velocity.value = tangentVelocity;
        debugState.frictionImpulse = frictionImpulse;
    }
    else
    {
        Vec3 lateralVelocity = horizontalVector(velocity.value);
        lateralVelocity =
            accelerateToward(lateralVelocity, desiredVelocity,
                             controller.airAcceleration * controller.airControl, fixedDeltaSeconds);
        velocity.value.x = lateralVelocity.x;
        velocity.value.z = lateralVelocity.z;
        velocity.value.y -= controller.gravity * fixedDeltaSeconds;
    }

    const bool canJump = touchingGround || grounding.coyoteTimeRemaining > 0.0f;
    if (controller.jumpBufferRemaining > 0.0f && canJump)
    {
        velocity.value = projectOntoPlane(velocity.value, supportNormal);
        velocity.value.y = controller.jumpSpeed;
        controller.jumpBufferRemaining = 0.0f;
        grounding.coyoteTimeRemaining = 0.0f;
    }

    const Vec3 fullDisplacement{velocity.value.x * fixedDeltaSeconds,
                                velocity.value.y * fixedDeltaSeconds,
                                velocity.value.z * fixedDeltaSeconds};
    MotionResult motion =
        sweepAndSlideCapsule(world, transform.position, fullDisplacement, velocity.value, shape,
                             controller.sweepSkinWidth, controller.maxCollisionIterations);

    presentation.previousBodyPosition = presentation.currentBodyPosition;
    transform.position = motion.position;
    presentation.currentBodyPosition = transform.position;
    velocity.value = motion.velocity;

    const SupportInfo support =
        queryGroundSupport(world, transform.position, shape, controller.supportProbeDistance,
                           controller.maxSlopeAngleDegrees);
    const bool wasGrounded = grounding.grounded;
    const float supportSeparationSpeed = support.walkable
                                             ? dot(velocity.value, support.normal)
                                             : dot(velocity.value, grounding.supportNormal);
    const bool directGrounded = support.walkable && supportSeparationSpeed <= 1.0f &&
                                support.distance <= controller.supportProbeDistance;
    if (directGrounded)
    {
        grounding.supportPersistenceRemaining = kSupportPersistenceSeconds;
    }
    else
    {
        grounding.supportPersistenceRemaining =
            std::max(grounding.supportPersistenceRemaining - fixedDeltaSeconds, 0.0f);
    }

    const bool retainedGrounded = wasGrounded && grounding.supportPersistenceRemaining > 0.0f &&
                                  support.walkable && support.distance <= supportRetainDistance &&
                                  supportSeparationSpeed <= kGroundRetainVerticalSpeed;
    const bool grounded = directGrounded || retainedGrounded;
    grounding.supportRetained = retainedGrounded && !directGrounded;

    if (grounded)
    {
        if (!wasGrounded && previousVelocity.y < -0.1f)
        {
            presentation.landingDip =
                std::min(presentation.landingDip + (-previousVelocity.y) * 0.004f, kMaxLandingDip);
        }
        if (dot(velocity.value, support.normal) < 0.0f)
        {
            velocity.value = projectOntoPlane(velocity.value, support.normal);
        }
        grounding.coyoteTimeRemaining = controller.coyoteTimeSeconds;
        grounding.groundedDuration += fixedDeltaSeconds;
        ++grounding.groundedFrames;
    }
    else
    {
        grounding.groundedDuration = 0.0f;
        grounding.groundedFrames = 0;
    }

    grounding.grounded = grounded;
    grounding.supportHit = support.hit || grounding.supportRetained;
    if (!grounding.supportRetained)
    {
        grounding.supportNormal = support.normal;
        grounding.supportPoint = support.point;
        grounding.supportHeight = support.height;
        grounding.supportDistance = support.distance;
        grounding.slopeAngleDegrees = support.slopeAngleDegrees;
    }
    grounding.penetrationRecoveries = motion.penetrationRecoveries;
    grounding.collisionCount = motion.collisionCount;
    grounding.sweepIterations = motion.iterations;
    if (grounded && !wasGrounded)
    {
        ++grounding.supportAcquisitionCount;
        ++grounding.groundedTransitionCount;
    }
    else if (!grounded && wasGrounded)
    {
        ++grounding.airborneTransitionCount;
    }

    controller.traversalState =
        grounded ? (controller.crouching ? components::PlayerTraversalState::Crouching
                                         : components::PlayerTraversalState::Grounded)
                 : components::PlayerTraversalState::Airborne;

    debugState.cursorCaptured = input.cursorCaptured;
    debugState.grounded = grounded;
    debugState.supportHit = grounding.supportHit;
    debugState.supportRetained = grounding.supportRetained;
    debugState.crouching = controller.crouching;
    debugState.stepUpApplied = false;
    debugState.inputDirection = intent;
    debugState.desiredVelocity = desiredVelocity;
    debugState.projectedVelocity = velocity.value;
    debugState.supportNormal = grounding.supportNormal;
    debugState.supportPoint = grounding.supportPoint;
    debugState.terrainNormal =
        sampleAtmosphericTerrainNormal(worldSettings, transform.position.x, transform.position.z);
    debugState.lastCollisionNormal = motion.lastCollisionNormal;
    debugState.lastSurfaceMotion = motion.lastSurfaceMotion;
    debugState.acceleration = (velocity.value - previousVelocity) / fixedDeltaSeconds;
    debugState.sweepStart = startPosition;
    debugState.sweepEnd = transform.position;
    debugState.postCollisionPosition = transform.position;
    debugState.velocity = velocity.value;
    debugState.terrainHeight =
        sampleAtmosphericTerrainHeight(worldSettings, transform.position.x, transform.position.z);
    debugState.supportHeight = grounding.supportHeight;
    debugState.supportDistance = grounding.supportDistance;
    debugState.slopeAngleDegrees = grounding.slopeAngleDegrees;
    debugState.friction = grounded ? controller.groundFriction : 0.0f;
    debugState.airControl = controller.airControl;
    debugState.capsuleRadius = collider.radius;
    debugState.capsuleHeight = collider.height;
    debugState.collisionCount = motion.collisionCount;
    debugState.penetrationRecoveries = motion.penetrationRecoveries;
    debugState.groundedDuration = grounding.groundedDuration;
    debugState.coyoteTimeRemaining = grounding.coyoteTimeRemaining;
    debugState.jumpBufferRemaining = controller.jumpBufferRemaining;
    debugState.supportPersistenceRemaining = grounding.supportPersistenceRemaining;
    debugState.supportAcquisitionCount = grounding.supportAcquisitionCount;
    debugState.collisionTriangleCount = static_cast<int>(world.triangles.size());
    debugState.sweepIterations = motion.iterations;
    debugState.groundedTransitionCount = grounding.groundedTransitionCount;
    debugState.airborneTransitionCount = grounding.airborneTransitionCount;
    debugState.horizontalMomentumRatio =
        length(horizontalVector(velocity.value)) /
        std::max(length(horizontalVector(previousVelocity)), 0.001f);
    debugState.residualMotionLength = motion.residualMotionLength;
    debugState.sweepFailureDetected = motion.exhaustedIterations;
}

void PlayerController::updatePresentation(Scene& scene, ecs::Entity playerEntity,
                                          float deltaSeconds, MovementDebugState& debugState)
{
    ecs::Registry& registry = scene.registry();
    const components::VelocityComponent& velocity =
        registry.get<components::VelocityComponent>(playerEntity);
    const components::PlayerInputComponent& input =
        registry.get<components::PlayerInputComponent>(playerEntity);
    const components::PlayerControllerComponent& controller =
        registry.get<components::PlayerControllerComponent>(playerEntity);
    const components::GroundingComponent& grounding =
        registry.get<components::GroundingComponent>(playerEntity);
    components::CameraPresentationComponent& presentation =
        registry.get<components::CameraPresentationComponent>(playerEntity);

    const float targetEyeHeight =
        controller.crouching ? presentation.crouchedEyeHeight : presentation.standingEyeHeight;
    presentation.currentEyeHeight =
        approachScalar(presentation.currentEyeHeight, targetEyeHeight, deltaSeconds * 3.5f);

    const float speed = length(horizontalVector(velocity.value));
    const float movementInputMagnitude = std::clamp(length(input.moveAxes), 0.0f, 1.0f);
    const float normalizedSpeed =
        std::clamp(speed / std::max(controller.walkSpeed, 0.1f), 0.0f, 1.0f);

    const float bobTarget =
        grounding.grounded ? movementInputMagnitude * movementInputMagnitude : 0.0f;
    presentation.bobBlend =
        approachScalar(presentation.bobBlend, bobTarget, deltaSeconds * m_cameraBobBlendSpeed);
    const float movementPresentationBlend = presentation.bobBlend * normalizedSpeed;

    presentation.breathingPhase += deltaSeconds * (grounding.grounded ? 0.85f : 0.45f);
    const float breathingOffset = std::sin(presentation.breathingPhase) * kBreathingAmplitude;

    float bobSide = 0.0f;
    float bobUp = 0.0f;
    float bobPitch = 0.0f;
    float bobRoll = 0.0f;
    if (presentation.bobBlend > 0.001f && speed > 0.08f)
    {
        presentation.bobPhase += deltaSeconds * (5.25f + normalizedSpeed * 1.4f);
        const float bobWave = std::sin(presentation.bobPhase);
        const float liftWave = std::sin(presentation.bobPhase * 2.0f + 0.45f);
        const float bobStrength = std::min(
            presentation.bobBlend * normalizedSpeed * m_cameraBobAmount, kMaxEffectiveBobStrength);

        bobSide = bobStrength * kBobSideAmplitude * bobWave;
        bobUp = bobStrength * kBobUpAmplitude * (0.65f + normalizedSpeed) * liftWave;
        bobPitch = bobStrength * kBobPitchAmplitudeDegrees * bobWave;
        bobRoll = bobStrength * kBobRollAmplitudeDegrees * bobWave;
    }

    presentation.landingDip = std::max(presentation.landingDip - deltaSeconds * 0.24f, 0.0f);

    const float targetPitch =
        -input.moveAxes.y * kMovePitchAmplitudeDegrees * movementPresentationBlend;
    const float targetRoll =
        -input.moveAxes.x * kMoveRollAmplitudeDegrees * movementPresentationBlend;
    presentation.pitchOffsetDegrees =
        approachScalar(presentation.pitchOffsetDegrees, targetPitch, deltaSeconds * 4.5f);
    presentation.rollDegrees =
        approachScalar(presentation.rollDegrees, targetRoll, deltaSeconds * 5.5f);
    presentation.bobPitchOffsetDegrees =
        approachScalar(presentation.bobPitchOffsetDegrees, bobPitch, deltaSeconds * 5.0f);
    presentation.bobRollDegrees =
        approachScalar(presentation.bobRollDegrees, bobRoll, deltaSeconds * 5.5f);

    presentation.localOffset =
        Vec3{bobSide, breathingOffset + bobUp - presentation.landingDip, 0.0f};
    debugState.cameraOffset = presentation.localOffset;
    debugState.headBobAmount = bobUp;
    debugState.landingDip = presentation.landingDip;
}

void PlayerController::syncPlayerFromEcs(const Scene& scene, ecs::Entity playerEntity,
                                         Player& player, float presentationAlpha) const
{
    const ecs::Registry& registry = scene.registry();
    const components::TransformComponent& transform =
        registry.get<components::TransformComponent>(playerEntity);
    const components::VelocityComponent& velocity =
        registry.get<components::VelocityComponent>(playerEntity);
    const components::ColliderComponent& collider =
        registry.get<components::ColliderComponent>(playerEntity);
    const components::GroundingComponent& grounding =
        registry.get<components::GroundingComponent>(playerEntity);
    const components::CameraComponent& camera =
        registry.get<components::CameraComponent>(playerEntity);
    const components::CameraPresentationComponent& presentation =
        registry.get<components::CameraPresentationComponent>(playerEntity);

    player.setCollisionShape(collider.radius, collider.height);
    player.setPosition(transform.position);
    player.setVelocity(velocity.value);
    player.setGrounded(grounding.grounded);
    player.setEyeHeight(presentation.currentEyeHeight);
    player.camera().setPerspective(camera.fieldOfViewRadians, camera.nearPlane, 160.0f);
    player.camera().setYawPitchRoll(
        camera.yawDegrees,
        camera.pitchDegrees + presentation.pitchOffsetDegrees + presentation.bobPitchOffsetDegrees,
        camera.rollDegrees + presentation.rollDegrees + presentation.bobRollDegrees);

    const float clampedAlpha = std::clamp(presentationAlpha, 0.0f, 1.0f);
    const Vec3 renderBodyPosition =
        lerpVec3(presentation.previousBodyPosition, presentation.currentBodyPosition, clampedAlpha);
    const Vec3 basePosition = renderBodyPosition + Vec3{0.0f, presentation.currentEyeHeight, 0.0f};
    const Vec3 offset = player.camera().right() * presentation.localOffset.x +
                        player.camera().up() * presentation.localOffset.y +
                        player.camera().front() * presentation.localOffset.z;
    player.camera().setPosition(basePosition + offset);
}

const CollisionWorld& PlayerController::collisionWorld(const Scene& scene,
                                                       const ProceduralWorldSettings& worldSettings)
{
    const std::size_t worldSignature = collisionWorldSignature(scene);
    const bool settingsChanged =
        !m_collisionCache.valid || m_collisionCache.terrainSeed != worldSettings.seed ||
        m_collisionCache.terrainDensity != worldSettings.terrainDensity ||
        std::abs(m_collisionCache.terrainScale - worldSettings.terrainScale) > 0.001f ||
        std::abs(m_collisionCache.terrainHeight - worldSettings.terrainHeight) > 0.001f;
    const bool signatureChanged =
        !m_collisionCache.valid || m_collisionCache.worldSignature != worldSignature;

    m_collisionCacheRebuilt = false;
    m_staleColliderDetected = false;

    const bool needsRebuild = settingsChanged || signatureChanged;

    if (needsRebuild)
    {
        m_collisionCache.world = buildCollisionWorld(scene);
        m_collisionCache.valid = true;
        m_collisionCache.worldSignature = worldSignature;
        m_collisionCache.terrainSeed = worldSettings.seed;
        m_collisionCache.terrainDensity = worldSettings.terrainDensity;
        m_collisionCache.terrainScale = worldSettings.terrainScale;
        m_collisionCache.terrainHeight = worldSettings.terrainHeight;
        m_collisionCacheRebuilt = true;
        m_staleColliderDetected = signatureChanged && !settingsChanged;
    }

    return m_collisionCache.world;
}
} // namespace engine
