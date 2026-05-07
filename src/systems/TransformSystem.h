#pragma once

#include "components/WorldComponents.h"
#include "math/Transform.h"

namespace engine
{
class Scene;
}

namespace engine::systems
{
class TransformSystem final
{
  public:
    static void updateWorldTransforms(Scene& scene);
    static Mat4 composeWorldMatrix(const components::TransformComponent& component);
    static Transform toLegacyTransform(const components::TransformComponent& component);
};
} // namespace engine::systems
