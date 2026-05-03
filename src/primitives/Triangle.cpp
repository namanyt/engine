#include "primitives/Triangle.h"

#include "geometry/Generators.h"

namespace engine
{
Triangle::Triangle()
    : m_mesh(makeTriangle())
{
}

void Triangle::draw() const
{
    m_mesh.draw();
}

const Mesh& Triangle::mesh() const noexcept
{
    return m_mesh;
}
} // namespace engine
