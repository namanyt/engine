#include "world/Player.h"

#include <algorithm>

namespace engine
{
Player::Player()
{
    syncCamera();
}

Player::Player(const Vec3& feetPosition) : m_position(feetPosition)
{
    syncCamera();
}

const Vec3& Player::position() const noexcept
{
    return m_position;
}

const Vec3& Player::velocity() const noexcept
{
    return m_velocity;
}

bool Player::grounded() const noexcept
{
    return m_grounded;
}

float Player::collisionRadius() const noexcept
{
    return m_collisionRadius;
}

float Player::collisionHeight() const noexcept
{
    return m_collisionHeight;
}

float Player::eyeHeight() const noexcept
{
    return m_eyeHeight;
}

void Player::setPosition(const Vec3& position) noexcept
{
    m_position = position;
    syncCamera();
}

void Player::setVelocity(const Vec3& velocity) noexcept
{
    m_velocity = velocity;
}

void Player::setGrounded(bool grounded) noexcept
{
    m_grounded = grounded;
}

void Player::setYawPitch(float yawDegrees, float pitchDegrees) noexcept
{
    m_camera.setYawPitch(yawDegrees, pitchDegrees);
    syncCamera();
}

void Player::rotate(float yawOffsetDegrees, float pitchOffsetDegrees) noexcept
{
    m_camera.rotate(yawOffsetDegrees, pitchOffsetDegrees);
    syncCamera();
}

void Player::setCollisionShape(float radius, float height) noexcept
{
    m_collisionRadius = std::max(radius, 0.1f);
    m_collisionHeight = std::max(height, 0.5f);
}

void Player::setEyeHeight(float eyeHeight) noexcept
{
    m_eyeHeight = std::clamp(eyeHeight, 0.5f, m_collisionHeight - 0.05f);
    syncCamera();
}

Camera& Player::camera() noexcept
{
    return m_camera;
}

const Camera& Player::camera() const noexcept
{
    return m_camera;
}

void Player::syncCamera() noexcept
{
    m_camera.setPosition(m_position + Vec3{0.0f, m_eyeHeight, 0.0f});
}
} // namespace engine
