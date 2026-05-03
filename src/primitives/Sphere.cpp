#include "primitives/Sphere.h"

#include "primitives/PrimitiveBuilders.h"

namespace engine
{
Sphere::Sphere(unsigned int stackCount, unsigned int sliceCount)
    : m_mesh(makeSphereGeometry(stackCount, sliceCount))
{
}

void Sphere::draw() const
{
    m_mesh.draw();
}

const Mesh& Sphere::mesh() const noexcept
{
    return m_mesh;
}
} // namespace engine
