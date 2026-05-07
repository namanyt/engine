#pragma once

#include "Application.h"

namespace engine
{
class Camera;

class FreeCameraController final
{
  public:
    void update(Camera& camera, const InputState& inputState, float deltaSeconds) const;

    void setMoveSpeed(float moveSpeed) noexcept;
    void setSprintMultiplier(float sprintMultiplier) noexcept;
    void setMouseSensitivity(float mouseSensitivity) noexcept;

  private:
    float m_moveSpeed = 9.0f;
    float m_sprintMultiplier = 1.9f;
    float m_mouseSensitivity = 0.10f;
};
} // namespace engine
