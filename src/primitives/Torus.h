#pragma once

#include "graphics/Mesh.h"

namespace engine
{
class Torus final
{
public:
    Torus(unsigned int majorSegments = 24, unsigned int minorSegments = 16);

    void draw() const;
    const Mesh& mesh() const noexcept;

private:
    Mesh m_mesh;
};
} // namespace engine
