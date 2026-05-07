#pragma once

#include "Application.h"
#include "world/WorldNavigation.h"

namespace engine
{
class Player;
class Scene;

class PlayerController final
{
  public:
    void update(Player& player, const Scene& scene, const InputState& inputState,
                float deltaSeconds) const;

    float walkSpeed() const noexcept;
    float sprintMultiplier() const noexcept;
    float jumpSpeed() const noexcept;
    float gravity() const noexcept;
    float mouseSensitivity() const noexcept;

    void setWalkSpeed(float walkSpeed) noexcept;
    void setSprintMultiplier(float sprintMultiplier) noexcept;
    void setJumpSpeed(float jumpSpeed) noexcept;
    void setGravity(float gravity) noexcept;
    void setMouseSensitivity(float mouseSensitivity) noexcept;

  private:
    float m_walkSpeed = 8.0f;
    float m_sprintMultiplier = 1.30f;
    float m_mouseSensitivity = 0.10f;
    float m_groundAcceleration = 18.0f;
    float m_airAcceleration = 4.5f;
    float m_jumpSpeed = 6.2f;
    float m_gravity = 16.0f;
    float m_groundSnapDistance = 0.26f;
    float m_maxStepHeight = 0.85f;
    float m_minGroundNormalY = 0.60f;
};
} // namespace engine
