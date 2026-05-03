#pragma once

#include "math/Types.h"

namespace engine
{
class Transform final
{
public:
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};

    Mat4 modelMatrix() const;
};
} // namespace engine
