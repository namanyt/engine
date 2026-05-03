#pragma once

#include "math/Types.h"

#include <vector>

namespace engine
{
struct Vertex final
{
    Vec3 position{};
    Vec3 normal{};
    Vec2 uv{};
};

struct Geometry final
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    bool empty() const noexcept;
};
} // namespace engine
