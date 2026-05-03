#include "geometry/Operations.h"

#include "math/Types.h"

#include <map>
#include <stdexcept>

namespace
{
struct EdgeKey final
{
    unsigned int first = 0;
    unsigned int second = 0;

    bool operator<(const EdgeKey& other) const noexcept
    {
        if (first != other.first)
        {
            return first < other.first;
        }

        return second < other.second;
    }
};

struct EdgeData final
{
    unsigned int count = 0;
    unsigned int start = 0;
    unsigned int end = 0;
};

engine::Vertex makeVertex(const engine::Vec3& position, const engine::Vec3& normal, const engine::Vec2& uv)
{
    engine::Vertex vertex{};
    vertex.position = position;
    vertex.normal = normal;
    vertex.uv = uv;
    return vertex;
}
} // namespace

namespace engine
{
Geometry extrude(const Geometry& source, float depth)
{
    if (source.empty())
    {
        return Geometry{};
    }

    const float halfDepth = depth * 0.5f;
    const unsigned int sourceVertexCount = static_cast<unsigned int>(source.vertices.size());

    Geometry result{};
    result.vertices.reserve(source.vertices.size() * 2U + source.indices.size() * 2U);
    result.indices.reserve(source.indices.size() * 2U + source.indices.size() * 4U);

    for (const Vertex& vertex : source.vertices)
    {
        result.vertices.push_back(makeVertex(
            Vec3{vertex.position.x, vertex.position.y, vertex.position.z + halfDepth},
            Vec3{0.0f, 0.0f, 1.0f},
            vertex.uv));
    }

    for (const unsigned int index : source.indices)
    {
        result.indices.push_back(index);
    }

    for (const Vertex& vertex : source.vertices)
    {
        result.vertices.push_back(makeVertex(
            Vec3{vertex.position.x, vertex.position.y, vertex.position.z - halfDepth},
            Vec3{0.0f, 0.0f, -1.0f},
            vertex.uv));
    }

    for (std::size_t triangle = 0; triangle < source.indices.size(); triangle += 3)
    {
        result.indices.push_back(sourceVertexCount + source.indices[triangle]);
        result.indices.push_back(sourceVertexCount + source.indices[triangle + 2]);
        result.indices.push_back(sourceVertexCount + source.indices[triangle + 1]);
    }

    std::map<EdgeKey, EdgeData> edgeMap;

    for (std::size_t triangle = 0; triangle < source.indices.size(); triangle += 3)
    {
        const unsigned int triangleIndices[3] = {
            source.indices[triangle],
            source.indices[triangle + 1],
            source.indices[triangle + 2],
        };

        for (int edge = 0; edge < 3; ++edge)
        {
            const unsigned int start = triangleIndices[edge];
            const unsigned int end = triangleIndices[(edge + 1) % 3];

            EdgeKey key{};
            key.first = start < end ? start : end;
            key.second = start < end ? end : start;

            EdgeData& data = edgeMap[key];
            if (data.count == 0)
            {
                data.start = start;
                data.end = end;
            }
            ++data.count;
        }
    }

    for (const auto& [key, data] : edgeMap)
    {
        (void)key;

        if (data.count != 1)
        {
            continue;
        }

        const Vec3 startPosition = source.vertices[data.start].position;
        const Vec3 endPosition = source.vertices[data.end].position;
        const Vec3 edgeDirection = normalize(endPosition - startPosition);
        const Vec3 sideNormal = normalize(Vec3{edgeDirection.y, -edgeDirection.x, 0.0f});

        if (length(sideNormal) <= 1.0e-6f)
        {
            throw std::runtime_error("Failed to extrude geometry because a boundary edge is degenerate.");
        }

        const unsigned int baseIndex = static_cast<unsigned int>(result.vertices.size());

        result.vertices.push_back(makeVertex(
            Vec3{startPosition.x, startPosition.y, startPosition.z + halfDepth},
            sideNormal,
            Vec2{0.0f, 0.0f}));
        result.vertices.push_back(makeVertex(
            Vec3{endPosition.x, endPosition.y, endPosition.z + halfDepth},
            sideNormal,
            Vec2{1.0f, 0.0f}));
        result.vertices.push_back(makeVertex(
            Vec3{endPosition.x, endPosition.y, endPosition.z - halfDepth},
            sideNormal,
            Vec2{1.0f, 1.0f}));
        result.vertices.push_back(makeVertex(
            Vec3{startPosition.x, startPosition.y, startPosition.z - halfDepth},
            sideNormal,
            Vec2{0.0f, 1.0f}));

        result.indices.push_back(baseIndex);
        result.indices.push_back(baseIndex + 1);
        result.indices.push_back(baseIndex + 2);
        result.indices.push_back(baseIndex + 2);
        result.indices.push_back(baseIndex + 3);
        result.indices.push_back(baseIndex);
    }

    return result;
}
} // namespace engine
