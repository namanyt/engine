#pragma once

#include "math/Types.h"

namespace engine
{
class Camera final
{
  public:
    Camera();
    explicit Camera(const Vec3& position);

    const Vec3& position() const noexcept;
    const Vec3& front() const noexcept;
    const Vec3& right() const noexcept;
    const Vec3& up() const noexcept;
    float yawDegrees() const noexcept;
    float pitchDegrees() const noexcept;
    float rollDegrees() const noexcept;
    float fieldOfViewRadians() const noexcept;
    float nearPlane() const noexcept;

    void setPosition(const Vec3& position) noexcept;
    void translate(const Vec3& offset) noexcept;
    void moveRelative(float forwardAmount, float rightAmount, float upAmount) noexcept;
    void setYawPitch(float yawDegrees, float pitchDegrees) noexcept;
    void setYawPitchRoll(float yawDegrees, float pitchDegrees, float rollDegrees) noexcept;
    void rotate(float yawOffsetDegrees, float pitchOffsetDegrees) noexcept;
    void setRollDegrees(float rollDegrees) noexcept;
    void setPerspective(float fieldOfViewRadians, float nearPlane, float farPlane) noexcept;

    Mat4 viewMatrix() const;
    Mat4 projectionMatrix(float aspectRatio) const;

  private:
    void updateBasis() noexcept;

    Vec3 m_position{0.0f, 1.8f, 13.5f};
    Vec3 m_worldUp{0.0f, 1.0f, 0.0f};
    Vec3 m_front{0.0f, 0.0f, -1.0f};
    Vec3 m_right{1.0f, 0.0f, 0.0f};
    Vec3 m_up{0.0f, 1.0f, 0.0f};
    float m_yawDegrees = -90.0f;
    float m_pitchDegrees = 0.0f;
    float m_rollDegrees = 0.0f;
    float m_fieldOfViewRadians = 0.78539816339f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 160.0f;
};
} // namespace engine
