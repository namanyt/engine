#pragma once

#include "geometry/Geometry.h"
#include "graphics/IndexBuffer.h"
#include "graphics/VertexArray.h"
#include "graphics/VertexBuffer.h"

#include "math/Types.h"

#include <vector>

namespace engine
{
class Mesh final
{
public:
    explicit Mesh(const Geometry& geometry);
    ~Mesh() = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh& operator=(Mesh&&) = delete;

    void draw() const;

    const Geometry& geometry() const noexcept;

private:
    struct MeshVertex final
    {
        Vec3 position{};
        Vec3 normal{};
        Vec2 uv{};
        Color color = Color::white();
    };

    static std::vector<MeshVertex> buildGpuVertices(const Geometry& geometry);

    Geometry m_geometry;
    std::vector<MeshVertex> m_gpuVertices;
    VertexArray m_vertexArray;
    VertexBuffer m_vertexBuffer;
    IndexBuffer m_indexBuffer;
};
} // namespace engine
