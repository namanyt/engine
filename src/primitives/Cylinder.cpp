#include "primitives/Cylinder.h"

#include "primitives/PrimitiveBuilders.h"

namespace engine
{
Cylinder::Cylinder(unsigned int segmentCount)
    : m_mesh(makeCylinderGeometry(segmentCount))
{
}

void Cylinder::draw() const
{
    m_mesh.draw();
}

const Mesh& Cylinder::mesh() const noexcept
{
    return m_mesh;
}
} // namespace engine
