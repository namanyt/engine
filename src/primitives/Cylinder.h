#pragma once

#include "graphics/Mesh.h"

namespace engine
{
class Cylinder final
{
public:
    explicit Cylinder(unsigned int segmentCount = 24);

    void draw() const;
    const Mesh& mesh() const noexcept;

private:
    Mesh m_mesh;
};
} // namespace engine
