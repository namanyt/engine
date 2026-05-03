#include "graphics/Mesh.h"

#include <glad/glad.h>

#include <cstddef>
#include <cmath>
#include <utility>

namespace engine
{
Mesh::Mesh(const Geometry& geometry)
    : m_geometry(geometry)
    , m_gpuVertices(buildGpuVertices(m_geometry))
    , m_vertexArray()
    , m_vertexBuffer(m_gpuVertices.data(), m_gpuVertices.size() * sizeof(MeshVertex))
    , m_indexBuffer(m_geometry.indices.data(), m_geometry.indices.size())
{
    m_vertexArray.bind();
    m_vertexBuffer.bind();
    m_indexBuffer.bind();

    m_vertexArray.setAttribute(0, 3, GL_FLOAT, false, sizeof(MeshVertex), offsetof(MeshVertex, position));
    m_vertexArray.setAttribute(1, 3, GL_FLOAT, false, sizeof(MeshVertex), offsetof(MeshVertex, normal));
    m_vertexArray.setAttribute(2, 2, GL_FLOAT, false, sizeof(MeshVertex), offsetof(MeshVertex, uv));
    m_vertexArray.setAttribute(3, 4, GL_FLOAT, false, sizeof(MeshVertex), offsetof(MeshVertex, color));

    VertexArray::unbind();
    VertexBuffer::unbind();
    IndexBuffer::unbind();
}

void Mesh::draw() const
{
    m_vertexArray.bind();
    m_indexBuffer.bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexBuffer.count()), GL_UNSIGNED_INT, nullptr);
}

const Geometry& Mesh::geometry() const noexcept
{
    return m_geometry;
}

std::vector<Mesh::MeshVertex> Mesh::buildGpuVertices(const Geometry& geometry)
{
    std::vector<MeshVertex> gpuVertices;
    gpuVertices.reserve(geometry.vertices.size());

    for (const Vertex& vertex : geometry.vertices)
    {
        MeshVertex gpuVertex{};
        gpuVertex.position = vertex.position;
        gpuVertex.normal = vertex.normal;
        gpuVertex.uv = vertex.uv;
        gpuVertex.color = Color{
            0.35f + 0.55f * std::fabs(vertex.normal.x),
            0.35f + 0.55f * std::fabs(vertex.normal.y),
            0.35f + 0.55f * std::fabs(vertex.normal.z),
            1.0f,
        };

        gpuVertices.push_back(gpuVertex);
    }

    return gpuVertices;
}
} // namespace engine
