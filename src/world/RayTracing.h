#pragma once

#include "math/Types.h"

#include <vector>

namespace engine
{
struct Ray final
{
    Vec3 origin{};
    Vec3 direction{0.0f, 0.0f, -1.0f};
};

struct HitResult final
{
    bool hit = false;
    float distance = 0.0f;
    Vec3 position{};
    Vec3 normal{};
};

struct BoundingSphere final
{
    Vec3 center{};
    float radius = 0.0f;
};

struct RayTracingScene final
{
    std::vector<BoundingSphere> bounds;
};
} // namespace engine
