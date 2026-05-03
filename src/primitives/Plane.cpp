#include "primitives/Plane.h"

#include "primitives/PrimitiveBuilders.h"

namespace engine
{
Plane::Plane()
    : m_mesh(makePlaneGeometry())
{
}

void Plane::draw() const
{
    m_mesh.draw();
}

const Mesh& Plane::mesh() const noexcept
{
    return m_mesh;
}
} // namespace engine
