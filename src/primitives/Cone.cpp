#include "primitives/Cone.h"

#include "primitives/PrimitiveBuilders.h"

namespace engine
{
Cone::Cone(unsigned int segmentCount)
    : m_mesh(makeConeGeometry(segmentCount))
{
}

void Cone::draw() const
{
    m_mesh.draw();
}

const Mesh& Cone::mesh() const noexcept
{
    return m_mesh;
}
} // namespace engine
