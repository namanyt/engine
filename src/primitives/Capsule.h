#pragma once

#include "graphics/Mesh.h"

namespace engine
{
class Capsule final
{
public:
    Capsule(unsigned int hemisphereSegments = 12, unsigned int ringSegments = 24);

    void draw() const;
    const Mesh& mesh() const noexcept;

private:
    Mesh m_mesh;
};
} // namespace engine
