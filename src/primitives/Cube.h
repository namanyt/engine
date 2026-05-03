#pragma once

#include "graphics/Mesh.h"

namespace engine
{
class Cube final
{
public:
    Cube();

    Cube(const Cube&) = delete;
    Cube& operator=(const Cube&) = delete;
    Cube(Cube&&) = delete;
    Cube& operator=(Cube&&) = delete;

    void draw() const;
    const Mesh& mesh() const noexcept;

private:
    Mesh m_mesh;
};
} // namespace engine
