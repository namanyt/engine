#pragma once

#include <array>

namespace engine
{
struct Vec2 final
{
    float x = 0.0f;
    float y = 0.0f;
};

struct Vec3 final
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Vec4 final
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

struct Color final
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    static Color white() noexcept;
    static Color black() noexcept;
};

struct Mat4 final
{
    std::array<float, 16> elements{};

    static Mat4 identity();

    const float* data() const noexcept;
    float* data() noexcept;
};

Vec2 operator+(const Vec2& left, const Vec2& right);
Vec2 operator-(const Vec2& left, const Vec2& right);
Vec2 operator*(const Vec2& value, float scalar);
Vec2 operator/(const Vec2& value, float scalar);

Vec3 operator+(const Vec3& left, const Vec3& right);
Vec3 operator-(const Vec3& left, const Vec3& right);
Vec3 operator-(const Vec3& value);
Vec3 operator*(const Vec3& value, float scalar);
Vec3 operator/(const Vec3& value, float scalar);

Vec4 operator+(const Vec4& left, const Vec4& right);
Vec4 operator*(const Vec4& value, float scalar);

float dot(const Vec2& left, const Vec2& right);
float dot(const Vec3& left, const Vec3& right);
Vec3 cross(const Vec3& left, const Vec3& right);

float length(const Vec2& value);
float length(const Vec3& value);
Vec2 normalize(const Vec2& value);
Vec3 normalize(const Vec3& value);

Mat4 operator*(const Mat4& left, const Mat4& right);

Mat4 makeTranslation(const Vec3& translation);
Mat4 makeScale(const Vec3& scale);
Mat4 makeRotationX(float angleRadians);
Mat4 makeRotationY(float angleRadians);
Mat4 makeRotationZ(float angleRadians);
Mat4 makePerspective(float fieldOfViewRadians, float aspectRatio, float nearPlane, float farPlane);
Mat4 makeOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
Mat4 makeLookAt(const Vec3& eye, const Vec3& target, const Vec3& upDirection);
} // namespace engine
