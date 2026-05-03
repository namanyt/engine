#include "geometry/Generators.h"

#include <cmath>
#include <stdexcept>

namespace
{
constexpr float kPi = 3.14159265359f;

engine::Vertex makeVertex(const engine::Vec3& position, const engine::Vec2& uv)
{
    engine::Vertex vertex{};
    vertex.position = position;
    vertex.normal = engine::Vec3{0.0f, 0.0f, 1.0f};
    vertex.uv = uv;
    return vertex;
}
} // namespace

namespace engine
{
Geometry makeTriangle()
{
    Geometry geometry{};
    geometry.vertices = {
        makeVertex(Vec3{0.0f, 0.5f, 0.0f}, Vec2{0.5f, 1.0f}),
        makeVertex(Vec3{-0.5f, -0.5f, 0.0f}, Vec2{0.0f, 0.0f}),
        makeVertex(Vec3{0.5f, -0.5f, 0.0f}, Vec2{1.0f, 0.0f}),
    };
    geometry.indices = {0, 1, 2};
    return geometry;
}

Geometry makeQuad()
{
    Geometry geometry{};
    geometry.vertices = {
        makeVertex(Vec3{-0.5f, -0.5f, 0.0f}, Vec2{0.0f, 0.0f}),
        makeVertex(Vec3{0.5f, -0.5f, 0.0f}, Vec2{1.0f, 0.0f}),
        makeVertex(Vec3{0.5f, 0.5f, 0.0f}, Vec2{1.0f, 1.0f}),
        makeVertex(Vec3{-0.5f, 0.5f, 0.0f}, Vec2{0.0f, 1.0f}),
    };
    geometry.indices = {0, 1, 2, 2, 3, 0};
    return geometry;
}

Geometry makeCircle(int segments)
{
    if (segments < 3)
    {
        throw std::runtime_error("Circle generator requires at least 3 segments.");
    }

    Geometry geometry{};
    geometry.vertices.reserve(static_cast<std::size_t>(segments) + 1U);
    geometry.indices.reserve(static_cast<std::size_t>(segments) * 3U);

    geometry.vertices.push_back(makeVertex(Vec3{0.0f, 0.0f, 0.0f}, Vec2{0.5f, 0.5f}));

    for (int segment = 0; segment < segments; ++segment)
    {
        const float ratio = static_cast<float>(segment) / static_cast<float>(segments);
        const float angle = ratio * kPi * 2.0f;
        const float x = 0.5f * std::cos(angle);
        const float y = 0.5f * std::sin(angle);

        geometry.vertices.push_back(makeVertex(
            Vec3{x, y, 0.0f},
            Vec2{x + 0.5f, y + 0.5f}));
    }

    for (int segment = 0; segment < segments; ++segment)
    {
        const unsigned int current = static_cast<unsigned int>(segment) + 1U;
        const unsigned int next = static_cast<unsigned int>((segment + 1) % segments) + 1U;

        geometry.indices.push_back(0);
        geometry.indices.push_back(current);
        geometry.indices.push_back(next);
    }

    return geometry;
}
} // namespace engine
