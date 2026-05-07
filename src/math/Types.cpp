#include "math/Types.h"

#include <cmath>

namespace
{
constexpr float kEpsilon = 1.0e-6f;
}

namespace engine
{
Color Color::white() noexcept
{
    return Color{1.0f, 1.0f, 1.0f, 1.0f};
}

Color Color::black() noexcept
{
    return Color{0.0f, 0.0f, 0.0f, 1.0f};
}

Mat4 Mat4::identity()
{
    Mat4 matrix{};
    matrix.elements[0] = 1.0f;
    matrix.elements[5] = 1.0f;
    matrix.elements[10] = 1.0f;
    matrix.elements[15] = 1.0f;
    return matrix;
}

const float* Mat4::data() const noexcept
{
    return elements.data();
}

float* Mat4::data() noexcept
{
    return elements.data();
}

Vec2 operator+(const Vec2& left, const Vec2& right)
{
    return Vec2{left.x + right.x, left.y + right.y};
}

Vec2 operator-(const Vec2& left, const Vec2& right)
{
    return Vec2{left.x - right.x, left.y - right.y};
}

Vec2 operator*(const Vec2& value, float scalar)
{
    return Vec2{value.x * scalar, value.y * scalar};
}

Vec2 operator/(const Vec2& value, float scalar)
{
    return Vec2{value.x / scalar, value.y / scalar};
}

Vec3 operator+(const Vec3& left, const Vec3& right)
{
    return Vec3{left.x + right.x, left.y + right.y, left.z + right.z};
}

Vec3 operator-(const Vec3& left, const Vec3& right)
{
    return Vec3{left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec3 operator-(const Vec3& value)
{
    return Vec3{-value.x, -value.y, -value.z};
}

Vec3 operator*(const Vec3& value, float scalar)
{
    return Vec3{value.x * scalar, value.y * scalar, value.z * scalar};
}

Vec3 operator/(const Vec3& value, float scalar)
{
    return Vec3{value.x / scalar, value.y / scalar, value.z / scalar};
}

Vec4 operator+(const Vec4& left, const Vec4& right)
{
    return Vec4{left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w};
}

Vec4 operator*(const Vec4& value, float scalar)
{
    return Vec4{value.x * scalar, value.y * scalar, value.z * scalar, value.w * scalar};
}

float dot(const Vec2& left, const Vec2& right)
{
    return left.x * right.x + left.y * right.y;
}

float dot(const Vec3& left, const Vec3& right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vec3 cross(const Vec3& left, const Vec3& right)
{
    return Vec3{
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x,
    };
}

float length(const Vec2& value)
{
    return std::sqrt(dot(value, value));
}

float length(const Vec3& value)
{
    return std::sqrt(dot(value, value));
}

Vec2 normalize(const Vec2& value)
{
    const float magnitude = length(value);
    if (magnitude <= kEpsilon)
    {
        return Vec2{};
    }

    return value / magnitude;
}

Vec3 normalize(const Vec3& value)
{
    const float magnitude = length(value);
    if (magnitude <= kEpsilon)
    {
        return Vec3{};
    }

    return value / magnitude;
}

Mat4 operator*(const Mat4& left, const Mat4& right)
{
    Mat4 result{};

    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            float value = 0.0f;

            for (int index = 0; index < 4; ++index)
            {
                value += left.elements[index * 4 + row] * right.elements[column * 4 + index];
            }

            result.elements[column * 4 + row] = value;
        }
    }

    return result;
}

Mat4 makeTranslation(const Vec3& translation)
{
    Mat4 matrix = Mat4::identity();
    matrix.elements[12] = translation.x;
    matrix.elements[13] = translation.y;
    matrix.elements[14] = translation.z;
    return matrix;
}

Mat4 makeScale(const Vec3& scale)
{
    Mat4 matrix{};
    matrix.elements[0] = scale.x;
    matrix.elements[5] = scale.y;
    matrix.elements[10] = scale.z;
    matrix.elements[15] = 1.0f;
    return matrix;
}

Mat4 makeRotationX(float angleRadians)
{
    const float cosine = std::cos(angleRadians);
    const float sine = std::sin(angleRadians);

    Mat4 matrix = Mat4::identity();
    matrix.elements[5] = cosine;
    matrix.elements[6] = sine;
    matrix.elements[9] = -sine;
    matrix.elements[10] = cosine;
    return matrix;
}

Mat4 makeRotationY(float angleRadians)
{
    const float cosine = std::cos(angleRadians);
    const float sine = std::sin(angleRadians);

    Mat4 matrix = Mat4::identity();
    matrix.elements[0] = cosine;
    matrix.elements[2] = -sine;
    matrix.elements[8] = sine;
    matrix.elements[10] = cosine;
    return matrix;
}

Mat4 makeRotationZ(float angleRadians)
{
    const float cosine = std::cos(angleRadians);
    const float sine = std::sin(angleRadians);

    Mat4 matrix = Mat4::identity();
    matrix.elements[0] = cosine;
    matrix.elements[1] = sine;
    matrix.elements[4] = -sine;
    matrix.elements[5] = cosine;
    return matrix;
}

Mat4 makePerspective(float fieldOfViewRadians, float aspectRatio, float nearPlane, float farPlane)
{
    const float tangent = std::tan(fieldOfViewRadians * 0.5f);
    const float inverseRange = 1.0f / (nearPlane - farPlane);

    Mat4 matrix{};
    matrix.elements[0] = 1.0f / (aspectRatio * tangent);
    matrix.elements[5] = 1.0f / tangent;
    matrix.elements[10] = (farPlane + nearPlane) * inverseRange;
    matrix.elements[11] = -1.0f;
    matrix.elements[14] = (2.0f * farPlane * nearPlane) * inverseRange;
    return matrix;
}

Mat4 makeInfinitePerspective(float fieldOfViewRadians, float aspectRatio, float nearPlane)
{
    const float tangent = std::tan(fieldOfViewRadians * 0.5f);

    Mat4 matrix{};
    matrix.elements[0] = 1.0f / (aspectRatio * tangent);
    matrix.elements[5] = 1.0f / tangent;
    matrix.elements[10] = -1.0f;
    matrix.elements[11] = -1.0f;
    matrix.elements[14] = -2.0f * nearPlane;
    return matrix;
}

Mat4 makeOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    Mat4 matrix = Mat4::identity();
    matrix.elements[0] = 2.0f / (right - left);
    matrix.elements[5] = 2.0f / (top - bottom);
    matrix.elements[10] = -2.0f / (farPlane - nearPlane);
    matrix.elements[12] = -(right + left) / (right - left);
    matrix.elements[13] = -(top + bottom) / (top - bottom);
    matrix.elements[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    return matrix;
}

Mat4 makeLookAt(const Vec3& eye, const Vec3& target, const Vec3& upDirection)
{
    const Vec3 forward = normalize(target - eye);
    const Vec3 side = normalize(cross(forward, upDirection));
    const Vec3 up = cross(side, forward);

    Mat4 matrix = Mat4::identity();
    matrix.elements[0] = side.x;
    matrix.elements[4] = side.y;
    matrix.elements[8] = side.z;
    matrix.elements[1] = up.x;
    matrix.elements[5] = up.y;
    matrix.elements[9] = up.z;
    matrix.elements[2] = -forward.x;
    matrix.elements[6] = -forward.y;
    matrix.elements[10] = -forward.z;
    matrix.elements[12] = -dot(side, eye);
    matrix.elements[13] = -dot(up, eye);
    matrix.elements[14] = dot(forward, eye);
    return matrix;
}
} // namespace engine
