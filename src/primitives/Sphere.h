#pragma once

#include "graphics/Mesh.h"

namespace engine
{
class Sphere final
{
public:
    Sphere(unsigned int stackCount = 16, unsigned int sliceCount = 24);

    void draw() const;
    const Mesh& mesh() const noexcept;

private:
    Mesh m_mesh;
};
} // namespace engine
