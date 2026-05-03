#pragma once

#include "graphics/Mesh.h"

namespace engine
{
class Triangle final
{
public:
    Triangle();

    void draw() const;
    const Mesh& mesh() const noexcept;

private:
    Mesh m_mesh;
};
} // namespace engine
