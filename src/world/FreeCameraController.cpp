#include "world/FreeCameraController.h"

#include "world/Camera.h"

namespace engine
{
void FreeCameraController::update(Camera& camera, const ExplorationInputState& inputState,
                                  float deltaSeconds) const
{
    if (deltaSeconds <= 0.0f)
    {
        return;
    }

    if (!inputState.cursorCaptured)
    {
        return;
    }

    camera.rotate(inputState.mouseDelta.x * m_mouseSensitivity,
                  inputState.mouseDelta.y * m_mouseSensitivity);

    Vec3 movement{};
    const Vec3 forward = normalize(Vec3{camera.front().x, 0.0f, camera.front().z});
    const Vec3 right = camera.right();

    if (inputState.moveForward)
    {
        movement = movement + forward;
    }

    if (inputState.moveBackward)
    {
        movement = movement - forward;
    }

    if (inputState.moveRight)
    {
        movement = movement + right;
    }

    if (inputState.moveLeft)
    {
        movement = movement - right;
    }

    if (inputState.moveUp)
    {
        movement = movement + Vec3{0.0f, 1.0f, 0.0f};
    }

    if (inputState.moveDown)
    {
        movement = movement - Vec3{0.0f, 1.0f, 0.0f};
    }

    if (length(movement) <= 0.0f)
    {
        return;
    }

    const float speedMultiplier = inputState.sprint ? m_sprintMultiplier : 1.0f;
    const Vec3 velocity = normalize(movement) * (m_moveSpeed * speedMultiplier * deltaSeconds);
    camera.translate(velocity);
}

void FreeCameraController::setMoveSpeed(float moveSpeed) noexcept
{
    m_moveSpeed = moveSpeed;
}

void FreeCameraController::setSprintMultiplier(float sprintMultiplier) noexcept
{
    m_sprintMultiplier = sprintMultiplier;
}

void FreeCameraController::setMouseSensitivity(float mouseSensitivity) noexcept
{
    m_mouseSensitivity = mouseSensitivity;
}
} // namespace engine
