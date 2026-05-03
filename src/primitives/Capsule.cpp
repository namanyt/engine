#include "primitives/Capsule.h"

#include "primitives/PrimitiveBuilders.h"

namespace engine
{
Capsule::Capsule(unsigned int hemisphereSegments, unsigned int ringSegments)
    : m_mesh(makeCapsuleGeometry(hemisphereSegments, ringSegments))
{
}

void Capsule::draw() const
{
    m_mesh.draw();
}

const Mesh& Capsule::mesh() const noexcept
{
    return m_mesh;
}
} // namespace engine
