#include "world/Camera.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kPi = 3.14159265359f;
constexpr float kMinimumAspectRatio = 0.01f;
constexpr float kMinimumNearPlane = 0.01f;
constexpr float kMinimumFarPlaneGap = 1.0f;

float radians(float degrees) noexcept
{
    return degrees * kPi / 180.0f;
}
} // namespace

namespace engine
{
Camera::Camera()
{
    updateBasis();
}

Camera::Camera(const Vec3& position) : m_position(position)
{
    updateBasis();
}

const Vec3& Camera::position() const noexcept
{
    return m_position;
}

const Vec3& Camera::front() const noexcept
{
    return m_front;
}

const Vec3& Camera::right() const noexcept
{
    return m_right;
}

const Vec3& Camera::up() const noexcept
{
    return m_up;
}

float Camera::yawDegrees() const noexcept
{
    return m_yawDegrees;
}

float Camera::pitchDegrees() const noexcept
{
    return m_pitchDegrees;
}

float Camera::rollDegrees() const noexcept
{
    return m_rollDegrees;
}

float Camera::fieldOfViewRadians() const noexcept
{
    return m_fieldOfViewRadians;
}

float Camera::nearPlane() const noexcept
{
    return m_nearPlane;
}

void Camera::setPosition(const Vec3& position) noexcept
{
    m_position = position;
}

void Camera::translate(const Vec3& offset) noexcept
{
    m_position = m_position + offset;
}

void Camera::moveRelative(float forwardAmount, float rightAmount, float upAmount) noexcept
{
    m_position =
        m_position + (m_front * forwardAmount) + (m_right * rightAmount) + (m_worldUp * upAmount);
}

void Camera::setYawPitch(float yawDegrees, float pitchDegrees) noexcept
{
    setYawPitchRoll(yawDegrees, pitchDegrees, m_rollDegrees);
}

void Camera::setYawPitchRoll(float yawDegrees, float pitchDegrees, float rollDegrees) noexcept
{
    constexpr float kPitchLimitDegrees = 89.0f;

    m_yawDegrees = yawDegrees;
    m_pitchDegrees = std::clamp(pitchDegrees, -kPitchLimitDegrees, kPitchLimitDegrees);
    m_rollDegrees = rollDegrees;
    updateBasis();
}

void Camera::rotate(float yawOffsetDegrees, float pitchOffsetDegrees) noexcept
{
    setYawPitch(m_yawDegrees + yawOffsetDegrees, m_pitchDegrees + pitchOffsetDegrees);
}

void Camera::setRollDegrees(float rollDegrees) noexcept
{
    setYawPitchRoll(m_yawDegrees, m_pitchDegrees, rollDegrees);
}

void Camera::setPerspective(float fieldOfViewRadians, float nearPlane, float farPlane) noexcept
{
    m_fieldOfViewRadians = fieldOfViewRadians;
    m_nearPlane = std::max(nearPlane, kMinimumNearPlane);
    m_farPlane = std::max(farPlane, m_nearPlane + kMinimumFarPlaneGap);
}

Mat4 Camera::viewMatrix() const
{
    return makeLookAt(m_position, m_position + m_front, m_up);
}

Mat4 Camera::projectionMatrix(float aspectRatio) const
{
    const float safeAspectRatio = std::max(aspectRatio, kMinimumAspectRatio);
    return makePerspective(m_fieldOfViewRadians, safeAspectRatio, m_nearPlane, m_farPlane);
}

void Camera::updateBasis() noexcept
{
    const float yawRadians = radians(m_yawDegrees);
    const float pitchRadians = radians(m_pitchDegrees);
    const float cosPitch = std::cos(pitchRadians);

    m_front = normalize(Vec3{
        std::cos(yawRadians) * cosPitch,
        std::sin(pitchRadians),
        std::sin(yawRadians) * cosPitch,
    });
    const Vec3 baseRight = normalize(cross(m_front, m_worldUp));
    const Vec3 baseUp = normalize(cross(baseRight, m_front));
    const float rollRadians = radians(m_rollDegrees);
    const float cosine = std::cos(rollRadians);
    const float sine = std::sin(rollRadians);

    m_right = normalize(baseRight * cosine + baseUp * sine);
    m_up = normalize(cross(m_right, m_front));
}
} // namespace engine
