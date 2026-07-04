#include "math/Types.h"
#include "math/Transform.h"

#include <cmath>
#include <iostream>
#include <string_view>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace
{
constexpr float kMathEpsilon = 1.0e-4f;

bool approxEqual(float a, float b)
{
    return std::abs(a - b) <= kMathEpsilon;
}

bool approxEqual(const engine::Vec2& a, const engine::Vec2& b)
{
    return approxEqual(a.x, b.x) && approxEqual(a.y, b.y);
}

bool approxEqual(const engine::Vec3& a, const engine::Vec3& b)
{
    return approxEqual(a.x, b.x) && approxEqual(a.y, b.y) && approxEqual(a.z, b.z);
}

bool approxEqual(const engine::Vec4& a, const engine::Vec4& b)
{
    return approxEqual(a.x, b.x) && approxEqual(a.y, b.y) && approxEqual(a.z, b.z) &&
           approxEqual(a.w, b.w);
}

// ============================================================================
// Vec2 Tests
// ============================================================================

int math_vec2_addition()
{
    const engine::Vec2 a{1.0f, 2.0f};
    const engine::Vec2 b{3.0f, 4.0f};
    const engine::Vec2 result = a + b;

    if (!approxEqual(result, engine::Vec2{4.0f, 6.0f}))
    {
        std::cerr << "Vec2 addition failed.\n";
        return 1;
    }

    return 0;
}

int math_vec2_subtraction()
{
    const engine::Vec2 a{5.0f, 7.0f};
    const engine::Vec2 b{2.0f, 3.0f};
    const engine::Vec2 result = a - b;

    if (!approxEqual(result, engine::Vec2{3.0f, 4.0f}))
    {
        std::cerr << "Vec2 subtraction failed.\n";
        return 1;
    }

    return 0;
}

int math_vec2_scalar_multiply()
{
    const engine::Vec2 v{2.0f, 3.0f};
    const engine::Vec2 result = v * 4.0f;

    if (!approxEqual(result, engine::Vec2{8.0f, 12.0f}))
    {
        std::cerr << "Vec2 scalar multiplication failed.\n";
        return 1;
    }

    return 0;
}

int math_vec2_scalar_divide()
{
    const engine::Vec2 v{10.0f, 8.0f};
    const engine::Vec2 result = v / 2.0f;

    if (!approxEqual(result, engine::Vec2{5.0f, 4.0f}))
    {
        std::cerr << "Vec2 scalar division failed.\n";
        return 1;
    }

    return 0;
}

int math_vec2_dot()
{
    const engine::Vec2 a{1.0f, 2.0f};
    const engine::Vec2 b{3.0f, 4.0f};
    const float result = engine::dot(a, b);

    if (!approxEqual(result, 11.0f))
    {
        std::cerr << "Vec2 dot product failed (expected 11.0, got " << result << ").\n";
        return 1;
    }

    return 0;
}

int math_vec2_length()
{
    const engine::Vec2 v{3.0f, 4.0f};
    const float result = engine::length(v);

    if (!approxEqual(result, 5.0f))
    {
        std::cerr << "Vec2 length failed (expected 5.0, got " << result << ").\n";
        return 1;
    }

    return 0;
}

int math_vec2_normalize()
{
    const engine::Vec2 v{3.0f, 4.0f};
    const engine::Vec2 result = engine::normalize(v);

    if (!approxEqual(result, engine::Vec2{0.6f, 0.8f}))
    {
        std::cerr << "Vec2 normalize failed.\n";
        return 1;
    }

    const engine::Vec2 zero = engine::normalize(engine::Vec2{0.0f, 0.0f});
    if (!approxEqual(zero, engine::Vec2{0.0f, 0.0f}))
    {
        std::cerr << "Vec2 normalize of zero vector failed.\n";
        return 1;
    }

    return 0;
}

// ============================================================================
// Vec3 Tests
// ============================================================================

int math_vec3_addition()
{
    const engine::Vec3 a{1.0f, 2.0f, 3.0f};
    const engine::Vec3 b{4.0f, 5.0f, 6.0f};
    const engine::Vec3 result = a + b;

    if (!approxEqual(result, engine::Vec3{5.0f, 7.0f, 9.0f}))
    {
        std::cerr << "Vec3 addition failed.\n";
        return 1;
    }

    return 0;
}

int math_vec3_subtraction()
{
    const engine::Vec3 a{5.0f, 7.0f, 9.0f};
    const engine::Vec3 b{2.0f, 3.0f, 4.0f};
    const engine::Vec3 result = a - b;

    if (!approxEqual(result, engine::Vec3{3.0f, 4.0f, 5.0f}))
    {
        std::cerr << "Vec3 subtraction failed.\n";
        return 1;
    }

    return 0;
}

int math_vec3_negation()
{
    const engine::Vec3 v{1.0f, -2.0f, 3.0f};
    const engine::Vec3 result = -v;

    if (!approxEqual(result, engine::Vec3{-1.0f, 2.0f, -3.0f}))
    {
        std::cerr << "Vec3 negation failed.\n";
        return 1;
    }

    return 0;
}

int math_vec3_scalar_multiply()
{
    const engine::Vec3 v{2.0f, 3.0f, 4.0f};
    const engine::Vec3 result = v * 2.5f;

    if (!approxEqual(result, engine::Vec3{5.0f, 7.5f, 10.0f}))
    {
        std::cerr << "Vec3 scalar multiplication failed.\n";
        return 1;
    }

    return 0;
}

int math_vec3_scalar_divide()
{
    const engine::Vec3 v{6.0f, 9.0f, 12.0f};
    const engine::Vec3 result = v / 3.0f;

    if (!approxEqual(result, engine::Vec3{2.0f, 3.0f, 4.0f}))
    {
        std::cerr << "Vec3 scalar division failed.\n";
        return 1;
    }

    return 0;
}

int math_vec3_dot()
{
    const engine::Vec3 a{1.0f, 2.0f, 3.0f};
    const engine::Vec3 b{4.0f, 5.0f, 6.0f};
    const float result = engine::dot(a, b);

    if (!approxEqual(result, 32.0f))
    {
        std::cerr << "Vec3 dot product failed (expected 32.0, got " << result << ").\n";
        return 1;
    }

    const engine::Vec3 x{1.0f, 0.0f, 0.0f};
    const engine::Vec3 y{0.0f, 1.0f, 0.0f};
    if (!approxEqual(engine::dot(x, y), 0.0f))
    {
        std::cerr << "Vec3 dot product of orthogonal vectors should be zero.\n";
        return 1;
    }

    return 0;
}

int math_vec3_cross()
{
    const engine::Vec3 x{1.0f, 0.0f, 0.0f};
    const engine::Vec3 y{0.0f, 1.0f, 0.0f};
    const engine::Vec3 result = engine::cross(x, y);

    if (!approxEqual(result, engine::Vec3{0.0f, 0.0f, 1.0f}))
    {
        std::cerr << "Vec3 cross product (x x y) should be z.\n";
        return 1;
    }

    const engine::Vec3 reversed = engine::cross(y, x);
    if (!approxEqual(reversed, engine::Vec3{0.0f, 0.0f, -1.0f}))
    {
        std::cerr << "Vec3 cross product should be anti-commutative.\n";
        return 1;
    }

    return 0;
}

int math_vec3_length()
{
    const engine::Vec3 v{1.0f, 2.0f, 2.0f};
    const float result = engine::length(v);

    if (!approxEqual(result, 3.0f))
    {
        std::cerr << "Vec3 length failed (expected 3.0, got " << result << ").\n";
        return 1;
    }

    return 0;
}

int math_vec3_normalize()
{
    const engine::Vec3 v{2.0f, 3.0f, 6.0f};
    const engine::Vec3 result = engine::normalize(v);

    if (!approxEqual(result.x, 2.0f / 7.0f) || !approxEqual(result.y, 3.0f / 7.0f) ||
        !approxEqual(result.z, 6.0f / 7.0f))
    {
        std::cerr << "Vec3 normalize failed.\n";
        return 1;
    }

    if (!approxEqual(engine::length(result), 1.0f))
    {
        std::cerr << "Normalized Vec3 should have unit length.\n";
        return 1;
    }

    return 0;
}

// ============================================================================
// Vec4 Tests
// ============================================================================

int math_vec4_addition()
{
    const engine::Vec4 a{1.0f, 2.0f, 3.0f, 4.0f};
    const engine::Vec4 b{5.0f, 6.0f, 7.0f, 8.0f};
    const engine::Vec4 result = a + b;

    if (!approxEqual(result, engine::Vec4{6.0f, 8.0f, 10.0f, 12.0f}))
    {
        std::cerr << "Vec4 addition failed.\n";
        return 1;
    }

    return 0;
}

int math_vec4_scalar_multiply()
{
    const engine::Vec4 v{1.0f, 2.0f, 3.0f, 4.0f};
    const engine::Vec4 result = v * 0.5f;

    if (!approxEqual(result, engine::Vec4{0.5f, 1.0f, 1.5f, 2.0f}))
    {
        std::cerr << "Vec4 scalar multiplication failed.\n";
        return 1;
    }

    return 0;
}

// ============================================================================
// Color Tests
// ============================================================================

int math_color_white()
{
    const engine::Color c = engine::Color::white();
    if (!approxEqual(c.r, 1.0f) || !approxEqual(c.g, 1.0f) || !approxEqual(c.b, 1.0f) ||
        !approxEqual(c.a, 1.0f))
    {
        std::cerr << "Color::white() should return (1,1,1,1).\n";
        return 1;
    }

    return 0;
}

int math_color_black()
{
    const engine::Color c = engine::Color::black();
    if (!approxEqual(c.r, 0.0f) || !approxEqual(c.g, 0.0f) || !approxEqual(c.b, 0.0f) ||
        !approxEqual(c.a, 1.0f))
    {
        std::cerr << "Color::black() should return (0,0,0,1).\n";
        return 1;
    }

    return 0;
}

// ============================================================================
// Mat4 Tests
// ============================================================================

int math_mat4_identity()
{
    const engine::Mat4 m = engine::Mat4::identity();

    for (int i = 0; i < 16; ++i)
    {
        const float expected = ((i % 5) == 0) ? 1.0f : 0.0f;
        if (!approxEqual(m.elements[i], expected))
        {
            std::cerr << "Mat4 identity diagonal element [" << i << "] failed.\n";
            return 1;
        }
    }

    return 0;
}

int math_mat4_multiplication()
{
    const engine::Mat4 identity = engine::Mat4::identity();
    const engine::Vec3 translation{1.0f, 2.0f, 3.0f};
    const engine::Mat4 t = engine::makeTranslation(translation);
    const engine::Mat4 result = identity * t;

    for (int i = 0; i < 16; ++i)
    {
        if (!approxEqual(result.elements[i], t.elements[i]))
        {
            std::cerr << "Identity * Translation should equal Translation.\n";
            return 1;
        }
    }

    return 0;
}

int math_mat4_translation()
{
    const engine::Vec3 t{5.0f, -3.0f, 2.0f};
    const engine::Mat4 m = engine::makeTranslation(t);

    if (!approxEqual(m.elements[12], 5.0f) || !approxEqual(m.elements[13], -3.0f) ||
        !approxEqual(m.elements[14], 2.0f))
    {
        std::cerr << "Mat4 translation components failed.\n";
        return 1;
    }

    if (!approxEqual(m.elements[0], 1.0f) || !approxEqual(m.elements[5], 1.0f) ||
        !approxEqual(m.elements[10], 1.0f) || !approxEqual(m.elements[15], 1.0f))
    {
        std::cerr << "Mat4 translation diagonal should be identity.\n";
        return 1;
    }

    return 0;
}

int math_mat4_scale()
{
    const engine::Vec3 s{2.0f, 3.0f, 0.5f};
    const engine::Mat4 m = engine::makeScale(s);

    if (!approxEqual(m.elements[0], 2.0f) || !approxEqual(m.elements[5], 3.0f) ||
        !approxEqual(m.elements[10], 0.5f) || !approxEqual(m.elements[15], 1.0f))
    {
        std::cerr << "Mat4 scale diagonal failed.\n";
        return 1;
    }

    return 0;
}

int math_mat4_rotation_x()
{
    const float angle = static_cast<float>(M_PI / 2.0f);
    const engine::Mat4 m = engine::makeRotationX(angle);

    if (!approxEqual(m.elements[5], 0.0f) || !approxEqual(m.elements[6], 1.0f) ||
        !approxEqual(m.elements[9], -1.0f) || !approxEqual(m.elements[10], 0.0f))
    {
        std::cerr << "Mat4 rotation X failed.\n";
        return 1;
    }

    return 0;
}

int math_mat4_rotation_y()
{
    const float angle = static_cast<float>(M_PI / 2.0f);
    const engine::Mat4 m = engine::makeRotationY(angle);

    if (!approxEqual(m.elements[0], 0.0f) || !approxEqual(m.elements[2], -1.0f) ||
        !approxEqual(m.elements[8], 1.0f) || !approxEqual(m.elements[10], 0.0f))
    {
        std::cerr << "Mat4 rotation Y failed.\n";
        return 1;
    }

    return 0;
}

int math_mat4_rotation_z()
{
    const float angle = static_cast<float>(M_PI / 2.0f);
    const engine::Mat4 m = engine::makeRotationZ(angle);

    if (!approxEqual(m.elements[0], 0.0f) || !approxEqual(m.elements[1], 1.0f) ||
        !approxEqual(m.elements[4], -1.0f) || !approxEqual(m.elements[5], 0.0f))
    {
        std::cerr << "Mat4 rotation Z failed.\n";
        return 1;
    }

    return 0;
}

int math_mat4_inverse()
{
    const engine::Vec3 t{3.0f, -2.0f, 5.0f};
    const engine::Mat4 m = engine::makeTranslation(t);
    const engine::Mat4 inv = engine::inverse(m);

    if (!approxEqual(inv.elements[12], -3.0f) || !approxEqual(inv.elements[13], 2.0f) ||
        !approxEqual(inv.elements[14], -5.0f))
    {
        std::cerr << "Mat4 inverse of translation failed.\n";
        return 1;
    }

    const engine::Mat4 product = m * inv;
    const engine::Mat4 identity = engine::Mat4::identity();
    for (int i = 0; i < 16; ++i)
    {
        if (!approxEqual(product.elements[i], identity.elements[i]))
        {
            std::cerr << "M * inv(M) should equal identity.\n";
            return 1;
        }
    }

    return 0;
}

int math_mat4_perspective()
{
    const float fov = static_cast<float>(M_PI / 4.0f);
    const float aspect = 16.0f / 9.0f;
    const engine::Mat4 m = engine::makePerspective(fov, aspect, 0.1f, 100.0f);

    if (m.elements[5] <= 0.0f)
    {
        std::cerr << "Perspective matrix element [5] should be positive.\n";
        return 1;
    }

    if (m.elements[11] >= 0.0f || m.elements[14] >= 0.0f)
    {
        std::cerr << "Perspective matrix depth elements should be negative.\n";
        return 1;
    }

    return 0;
}

int math_mat4_orthographic()
{
    const engine::Mat4 m = engine::makeOrthographic(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    if (!approxEqual(m.elements[0], 1.0f) || !approxEqual(m.elements[5], 1.0f) ||
        !approxEqual(m.elements[10], -1.0f) || !approxEqual(m.elements[15], 1.0f))
    {
        std::cerr << "Orthographic matrix diagonal failed.\n";
        return 1;
    }

    return 0;
}

int math_mat4_look_at()
{
    const engine::Vec3 eye{0.0f, 0.0f, 5.0f};
    const engine::Vec3 target{0.0f, 0.0f, 0.0f};
    const engine::Vec3 up{0.0f, 1.0f, 0.0f};

    const engine::Mat4 m = engine::makeLookAt(eye, target, up);

    if (!approxEqual(m.elements[2], 0.0f) || !approxEqual(m.elements[6], 0.0f) ||
        !approxEqual(m.elements[10], 1.0f))
    {
        std::cerr << "LookAt forward direction failed.\n";
        return 1;
    }

    if (!approxEqual(m.elements[12], 0.0f) || !approxEqual(m.elements[13], 0.0f) ||
        !approxEqual(m.elements[14], -5.0f))
    {
        std::cerr << "LookAt translation failed.\n";
        return 1;
    }

    return 0;
}

// ============================================================================
// Transform Tests
// ============================================================================

int math_transform_model_matrix()
{
    engine::Transform t;
    t.position = engine::Vec3{2.0f, 3.0f, 4.0f};
    t.rotation = engine::Vec3{0.0f, 0.0f, 0.0f};
    t.scale = engine::Vec3{1.0f, 1.0f, 1.0f};

    const engine::Mat4 m = t.modelMatrix();

    if (!approxEqual(m.elements[12], 2.0f) || !approxEqual(m.elements[13], 3.0f) ||
        !approxEqual(m.elements[14], 4.0f))
    {
        std::cerr << "Transform model matrix translation failed.\n";
        return 1;
    }

    return 0;
}

int math_transform_scale()
{
    engine::Transform t;
    t.position = engine::Vec3{0.0f, 0.0f, 0.0f};
    t.rotation = engine::Vec3{0.0f, 0.0f, 0.0f};
    t.scale = engine::Vec3{2.0f, 3.0f, 0.5f};

    const engine::Mat4 m = t.modelMatrix();

    if (!approxEqual(m.elements[0], 2.0f) || !approxEqual(m.elements[5], 3.0f) ||
        !approxEqual(m.elements[10], 0.5f))
    {
        std::cerr << "Transform model matrix scale failed.\n";
        return 1;
    }

    return 0;
}

int math_transform_combined()
{
    engine::Transform t;
    t.position = engine::Vec3{1.0f, 0.0f, 0.0f};
    t.rotation = engine::Vec3{0.0f, 0.0f, 0.0f};
    t.scale = engine::Vec3{2.0f, 2.0f, 2.0f};

    const engine::Mat4 m = t.modelMatrix();

    if (!approxEqual(m.elements[0], 2.0f) || !approxEqual(m.elements[5], 2.0f) ||
        !approxEqual(m.elements[10], 2.0f))
    {
        std::cerr << "Transform combined scale diagonal failed.\n";
        return 1;
    }

    if (!approxEqual(m.elements[12], 1.0f))
    {
        std::cerr << "Transform combined translation failed.\n";
        return 1;
    }

    return 0;
}

// ============================================================================
// Dispatch Table
// ============================================================================

struct NamedTest
{
    std::string_view name;
    int (*function)();
};
} // namespace

int main(int argc, char** argv)
{
    const std::vector<NamedTest> tests = {
        {"math_vec2_addition", &math_vec2_addition},
        {"math_vec2_subtraction", &math_vec2_subtraction},
        {"math_vec2_scalar_multiply", &math_vec2_scalar_multiply},
        {"math_vec2_scalar_divide", &math_vec2_scalar_divide},
        {"math_vec2_dot", &math_vec2_dot},
        {"math_vec2_length", &math_vec2_length},
        {"math_vec2_normalize", &math_vec2_normalize},
        {"math_vec3_addition", &math_vec3_addition},
        {"math_vec3_subtraction", &math_vec3_subtraction},
        {"math_vec3_negation", &math_vec3_negation},
        {"math_vec3_scalar_multiply", &math_vec3_scalar_multiply},
        {"math_vec3_scalar_divide", &math_vec3_scalar_divide},
        {"math_vec3_dot", &math_vec3_dot},
        {"math_vec3_cross", &math_vec3_cross},
        {"math_vec3_length", &math_vec3_length},
        {"math_vec3_normalize", &math_vec3_normalize},
        {"math_vec4_addition", &math_vec4_addition},
        {"math_vec4_scalar_multiply", &math_vec4_scalar_multiply},
        {"math_color_white", &math_color_white},
        {"math_color_black", &math_color_black},
        {"math_mat4_identity", &math_mat4_identity},
        {"math_mat4_multiplication", &math_mat4_multiplication},
        {"math_mat4_translation", &math_mat4_translation},
        {"math_mat4_scale", &math_mat4_scale},
        {"math_mat4_rotation_x", &math_mat4_rotation_x},
        {"math_mat4_rotation_y", &math_mat4_rotation_y},
        {"math_mat4_rotation_z", &math_mat4_rotation_z},
        {"math_mat4_inverse", &math_mat4_inverse},
        {"math_mat4_perspective", &math_mat4_perspective},
        {"math_mat4_orthographic", &math_mat4_orthographic},
        {"math_mat4_look_at", &math_mat4_look_at},
        {"math_transform_model_matrix", &math_transform_model_matrix},
        {"math_transform_scale", &math_transform_scale},
        {"math_transform_combined", &math_transform_combined}};

    if (argc != 2)
    {
        std::cerr << "Usage: MathTests <test-name>\n";
        return 1;
    }

    const std::string_view requestedTest = argv[1];

    for (const NamedTest& test : tests)
    {
        if (test.name == requestedTest)
        {
            return test.function();
        }
    }

    std::cerr << "Unknown test: " << requestedTest << '\n';
    return 1;
}
