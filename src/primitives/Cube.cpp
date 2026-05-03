#include "primitives/Cube.h"

#include "primitives/PrimitiveBuilders.h"

namespace engine
{
Cube::Cube()
    : m_mesh(makeCubeGeometry())
{
}

void Cube::draw() const
{
    m_mesh.draw();
}

const Mesh& Cube::mesh() const noexcept
{
    return m_mesh;
}
} // namespace engine
