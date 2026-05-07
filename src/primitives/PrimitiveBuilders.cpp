#include "primitives/PrimitiveBuilders.h"

#include "geometry/Generators.h"
#include "geometry/Operations.h"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace
{
constexpr float kPi = 3.14159265359f;

engine::Vertex makeVertex(
    const engine::Vec3& position,
    const engine::Vec3& normal,
    const engine::Vec2& texCoord)
{
    engine::Vertex vertex{};
    vertex.position = position;
    vertex.normal = normal;
    vertex.uv = texCoord;
    return vertex;
}

engine::Vec3 rotateZUpToYUp(const engine::Vec3& value)
{
    return engine::Vec3{value.x, value.z, -value.y};
}

void appendQuad(
    std::vector<unsigned int>& indices,
    unsigned int bottomLeft,
    unsigned int bottomRight,
    unsigned int topRight,
    unsigned int topLeft)
{
    indices.push_back(bottomLeft);
    indices.push_back(bottomRight);
    indices.push_back(topRight);
    indices.push_back(topRight);
    indices.push_back(topLeft);
    indices.push_back(bottomLeft);
}

void appendSphereSection(
    std::vector<engine::Vertex>& vertices,
    std::vector<unsigned int>& indices,
    float radius,
    float yOffset,
    float phiStart,
    float phiEnd,
    unsigned int stackCount,
    unsigned int sliceCount,
    bool connectLastRing)
{
    const unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

    for (unsigned int stack = 0; stack <= stackCount; ++stack)
    {
        const float v = static_cast<float>(stack) / static_cast<float>(stackCount);
        const float phi = phiStart + (phiEnd - phiStart) * v;
        const float ringRadius = radius * std::sin(phi);
        const float y = yOffset + radius * std::cos(phi);

        for (unsigned int slice = 0; slice <= sliceCount; ++slice)
        {
            const float u = static_cast<float>(slice) / static_cast<float>(sliceCount);
            const float theta = u * kPi * 2.0f;

            const engine::Vec3 position{
                ringRadius * std::cos(theta),
                y,
                ringRadius * std::sin(theta),
            };

            const engine::Vec3 sphereCenter{0.0f, yOffset, 0.0f};
            const engine::Vec3 normal = engine::normalize(position - sphereCenter);

            vertices.push_back(makeVertex(position, normal, engine::Vec2{u, v}));
        }
    }

    const unsigned int ringVertexCount = sliceCount + 1;
    const unsigned int lastStack = connectLastRing ? stackCount : stackCount - 1;

    for (unsigned int stack = 0; stack < lastStack; ++stack)
    {
        for (unsigned int slice = 0; slice < sliceCount; ++slice)
        {
            const unsigned int current = baseIndex + stack * ringVertexCount + slice;
            const unsigned int next = current + ringVertexCount;

            appendQuad(indices, current, current + 1, next + 1, next);
        }
    }
}
} // namespace

namespace engine
{
Geometry makePlaneGeometry()
{
    Geometry quad = makeQuad();

    for (Vertex& vertex : quad.vertices)
    {
        vertex.position = rotateZUpToYUp(vertex.position);
        vertex.normal = rotateZUpToYUp(vertex.normal);
    }

    return quad;
}

Geometry makeCubeGeometry()
{
    return extrude(makeQuad(), 1.0f);
}

Geometry makeCylinderGeometry(unsigned int segmentCount)
{
    Geometry cylinder = extrude(makeCircle(static_cast<int>(segmentCount)), 1.0f);

    for (Vertex& vertex : cylinder.vertices)
    {
        vertex.position = rotateZUpToYUp(vertex.position);
        vertex.normal = rotateZUpToYUp(vertex.normal);
    }

    return cylinder;
}

Geometry makePyramidGeometry()
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const Vec3 apex{0.0f, 0.5f, 0.0f};
    const Vec3 base0{-0.5f, -0.5f,  0.5f};
    const Vec3 base1{ 0.5f, -0.5f,  0.5f};
    const Vec3 base2{ 0.5f, -0.5f, -0.5f};
    const Vec3 base3{-0.5f, -0.5f, -0.5f};

    auto appendFace = [&](const Vec3& a, const Vec3& b, const Vec3& c)
    {
        const Vec3 normal = normalize(cross(b - a, c - a));
        const unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

        vertices.push_back(makeVertex(a, normal, Vec2{0.5f, 1.0f}));
        vertices.push_back(makeVertex(b, normal, Vec2{0.0f, 0.0f}));
        vertices.push_back(makeVertex(c, normal, Vec2{1.0f, 0.0f}));

        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
    };

    appendFace(apex, base0, base1);
    appendFace(apex, base1, base2);
    appendFace(apex, base2, base3);
    appendFace(apex, base3, base0);

    const Vec3 baseNormal{0.0f, -1.0f, 0.0f};
    const unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

    vertices.push_back(makeVertex(base0, baseNormal, Vec2{0.0f, 0.0f}));
    vertices.push_back(makeVertex(base1, baseNormal, Vec2{1.0f, 0.0f}));
    vertices.push_back(makeVertex(base2, baseNormal, Vec2{1.0f, 1.0f}));
    vertices.push_back(makeVertex(base3, baseNormal, Vec2{0.0f, 1.0f}));

    indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 2, baseIndex + 3, baseIndex});

    Geometry geometry{};
    geometry.vertices = std::move(vertices);
    geometry.indices = std::move(indices);
    return geometry;
}

Geometry makeSphereGeometry(unsigned int stackCount, unsigned int sliceCount)
{
    if (stackCount < 3 || sliceCount < 3)
    {
        throw std::runtime_error("Sphere primitive requires at least 3 stacks and 3 slices.");
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int stack = 0; stack <= stackCount; ++stack)
    {
        const float v = static_cast<float>(stack) / static_cast<float>(stackCount);
        const float phi = v * kPi;

        for (unsigned int slice = 0; slice <= sliceCount; ++slice)
        {
            const float u = static_cast<float>(slice) / static_cast<float>(sliceCount);
            const float theta = u * kPi * 2.0f;

            const Vec3 position{
                0.5f * std::sin(phi) * std::cos(theta),
                0.5f * std::cos(phi),
                0.5f * std::sin(phi) * std::sin(theta),
            };

            const Vec3 normal = normalize(position);
            vertices.push_back(makeVertex(position, normal, Vec2{u, v}));
        }
    }

    const unsigned int ringVertexCount = sliceCount + 1;
    for (unsigned int stack = 0; stack < stackCount; ++stack)
    {
        for (unsigned int slice = 0; slice < sliceCount; ++slice)
        {
            const unsigned int current = stack * ringVertexCount + slice;
            const unsigned int next = current + ringVertexCount;
            appendQuad(indices, current, current + 1, next + 1, next);
        }
    }

    Geometry geometry{};
    geometry.vertices = std::move(vertices);
    geometry.indices = std::move(indices);
    return geometry;
}

Geometry makeConeGeometry(unsigned int segmentCount)
{
    if (segmentCount < 3)
    {
        throw std::runtime_error("Cone primitive requires at least 3 segments.");
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const Vec3 apex{0.0f, 0.5f, 0.0f};

    for (unsigned int segment = 0; segment < segmentCount; ++segment)
    {
        const float u0 = static_cast<float>(segment) / static_cast<float>(segmentCount);
        const float u1 = static_cast<float>(segment + 1) / static_cast<float>(segmentCount);
        const float angle0 = u0 * kPi * 2.0f;
        const float angle1 = u1 * kPi * 2.0f;

        const Vec3 base0{0.5f * std::cos(angle0), -0.5f, 0.5f * std::sin(angle0)};
        const Vec3 base1{0.5f * std::cos(angle1), -0.5f, 0.5f * std::sin(angle1)};
        const Vec3 normal = normalize(cross(base1 - apex, base0 - apex));
        const unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

        vertices.push_back(makeVertex(apex, normal, Vec2{0.5f, 1.0f}));
        vertices.push_back(makeVertex(base0, normal, Vec2{u0, 0.0f}));
        vertices.push_back(makeVertex(base1, normal, Vec2{u1, 0.0f}));

        indices.insert(indices.end(), {baseIndex, baseIndex + 1, baseIndex + 2});
    }

    const unsigned int bottomCenterIndex = static_cast<unsigned int>(vertices.size());
    vertices.push_back(makeVertex(Vec3{0.0f, -0.5f, 0.0f}, Vec3{0.0f, -1.0f, 0.0f}, Vec2{0.5f, 0.5f}));

    for (unsigned int segment = 0; segment <= segmentCount; ++segment)
    {
        const float u = static_cast<float>(segment) / static_cast<float>(segmentCount);
        const float angle = u * kPi * 2.0f;
        const float x = 0.5f * std::cos(angle);
        const float z = 0.5f * std::sin(angle);
        vertices.push_back(makeVertex(Vec3{x, -0.5f, z}, Vec3{0.0f, -1.0f, 0.0f}, Vec2{x + 0.5f, z + 0.5f}));
    }

    for (unsigned int segment = 0; segment < segmentCount; ++segment)
    {
        indices.push_back(bottomCenterIndex);
        indices.push_back(bottomCenterIndex + segment + 2);
        indices.push_back(bottomCenterIndex + segment + 1);
    }

    Geometry geometry{};
    geometry.vertices = std::move(vertices);
    geometry.indices = std::move(indices);
    return geometry;
}

Geometry makeCapsuleGeometry(unsigned int hemisphereSegments, unsigned int ringSegments)
{
    if (hemisphereSegments < 2 || ringSegments < 3)
    {
        throw std::runtime_error("Capsule primitive requires at least 2 hemisphere segments and 3 ring segments.");
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float radius = 0.25f;
    const float bodyHalfHeight = 0.25f;

    appendSphereSection(vertices, indices, radius, bodyHalfHeight, 0.0f, kPi * 0.5f, hemisphereSegments, ringSegments, false);

    const unsigned int cylinderBase = static_cast<unsigned int>(vertices.size());
    for (unsigned int ring = 0; ring <= 1; ++ring)
    {
        const float y = ring == 0 ? bodyHalfHeight : -bodyHalfHeight;
        const float v = ring == 0 ? 0.0f : 1.0f;

        for (unsigned int slice = 0; slice <= ringSegments; ++slice)
        {
            const float u = static_cast<float>(slice) / static_cast<float>(ringSegments);
            const float angle = u * kPi * 2.0f;
            const float x = radius * std::cos(angle);
            const float z = radius * std::sin(angle);
            const Vec3 normal = normalize(Vec3{x, 0.0f, z});

            vertices.push_back(makeVertex(Vec3{x, y, z}, normal, Vec2{u, v}));
        }
    }

    for (unsigned int slice = 0; slice < ringSegments; ++slice)
    {
        const unsigned int current = cylinderBase + slice;
        const unsigned int next = current + ringSegments + 1;
        appendQuad(indices, current, current + 1, next + 1, next);
    }

    appendSphereSection(vertices, indices, radius, -bodyHalfHeight, kPi * 0.5f, kPi, hemisphereSegments, ringSegments, true);

    Geometry geometry{};
    geometry.vertices = std::move(vertices);
    geometry.indices = std::move(indices);
    return geometry;
}

Geometry makeTorusGeometry(unsigned int majorSegments, unsigned int minorSegments)
{
    if (majorSegments < 3 || minorSegments < 3)
    {
        throw std::runtime_error("Torus primitive requires at least 3 major and minor segments.");
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const float majorRadius = 0.35f;
    const float minorRadius = 0.15f;

    for (unsigned int major = 0; major <= majorSegments; ++major)
    {
        const float u = static_cast<float>(major) / static_cast<float>(majorSegments);
        const float majorAngle = u * kPi * 2.0f;
        const float majorCos = std::cos(majorAngle);
        const float majorSin = std::sin(majorAngle);

        for (unsigned int minor = 0; minor <= minorSegments; ++minor)
        {
            const float v = static_cast<float>(minor) / static_cast<float>(minorSegments);
            const float minorAngle = v * kPi * 2.0f;
            const float minorCos = std::cos(minorAngle);
            const float minorSin = std::sin(minorAngle);

            const float ringRadius = majorRadius + minorRadius * minorCos;
            const Vec3 position{
                ringRadius * majorCos,
                minorRadius * minorSin,
                ringRadius * majorSin,
            };

            const Vec3 center{majorRadius * majorCos, 0.0f, majorRadius * majorSin};
            const Vec3 normal = normalize(position - center);
            vertices.push_back(makeVertex(position, normal, Vec2{u, v}));
        }
    }

    const unsigned int ringVertexCount = minorSegments + 1;
    for (unsigned int major = 0; major < majorSegments; ++major)
    {
        for (unsigned int minor = 0; minor < minorSegments; ++minor)
        {
            const unsigned int current = major * ringVertexCount + minor;
            const unsigned int next = current + ringVertexCount;
            appendQuad(indices, current, current + 1, next + 1, next);
        }
    }

    Geometry geometry{};
    geometry.vertices = std::move(vertices);
    geometry.indices = std::move(indices);
    return geometry;
}
} // namespace engine
