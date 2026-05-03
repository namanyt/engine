#pragma once

#include "graphics/Mesh.h"

namespace engine
{
class Plane final
{
public:
    Plane();

    void draw() const;
    const Mesh& mesh() const noexcept;

private:
    Mesh m_mesh;
};
} // namespace engine
