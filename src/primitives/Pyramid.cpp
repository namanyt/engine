#include "primitives/Pyramid.h"

#include "primitives/PrimitiveBuilders.h"

namespace engine
{
Pyramid::Pyramid()
    : m_mesh(makePyramidGeometry())
{
}

void Pyramid::draw() const
{
    m_mesh.draw();
}

const Mesh& Pyramid::mesh() const noexcept
{
    return m_mesh;
}
} // namespace engine
