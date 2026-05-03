#pragma once

#include "graphics/Mesh.h"

namespace engine
{
class Pyramid final
{
public:
    Pyramid();

    void draw() const;
    const Mesh& mesh() const noexcept;

private:
    Mesh m_mesh;
};
} // namespace engine
