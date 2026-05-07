#pragma once

#include "world/Camera.h"

namespace engine
{
class Player final
{
  public:
    Player();
    explicit Player(const Vec3& feetPosition);

    const Vec3& position() const noexcept;
    const Vec3& velocity() const noexcept;
    bool grounded() const noexcept;
    float collisionRadius() const noexcept;
    float collisionHeight() const noexcept;
    float eyeHeight() const noexcept;

    void setPosition(const Vec3& position) noexcept;
    void setVelocity(const Vec3& velocity) noexcept;
    void setGrounded(bool grounded) noexcept;
    void setYawPitch(float yawDegrees, float pitchDegrees) noexcept;
    void rotate(float yawOffsetDegrees, float pitchOffsetDegrees) noexcept;
    void setCollisionShape(float radius, float height) noexcept;
    void setEyeHeight(float eyeHeight) noexcept;

    Camera& camera() noexcept;
    const Camera& camera() const noexcept;

    void syncCamera() noexcept;

  private:
    Vec3 m_position{0.0f, 0.0f, 0.0f};
    Vec3 m_velocity{};
    Camera m_camera{};
    bool m_grounded = true;
    float m_collisionRadius = 0.42f;
    float m_collisionHeight = 1.8f;
    float m_eyeHeight = 1.64f;
};
} // namespace engine
