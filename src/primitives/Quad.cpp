#include "primitives/Quad.h"

#include "geometry/Generators.h"

namespace engine
{
Quad::Quad()
    : m_mesh(makeQuad())
{
}

void Quad::draw() const
{
    m_mesh.draw();
}

const Mesh& Quad::mesh() const noexcept
{
    return m_mesh;
}
} // namespace engine
