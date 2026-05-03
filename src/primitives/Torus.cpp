#include "primitives/Torus.h"

#include "primitives/PrimitiveBuilders.h"

namespace engine
{
Torus::Torus(unsigned int majorSegments, unsigned int minorSegments)
    : m_mesh(makeTorusGeometry(majorSegments, minorSegments))
{
}

void Torus::draw() const
{
    m_mesh.draw();
}

const Mesh& Torus::mesh() const noexcept
{
    return m_mesh;
}
} // namespace engine
