#include "systems/TransformSystem.h"

#include "world/Scene.h"

namespace engine::systems
{
void TransformSystem::updateWorldTransforms(Scene& scene)
{
    scene.registry().forEach<components::TransformComponent>(
        [](ecs::Entity, components::TransformComponent& transform)
        { transform.worldMatrix = composeWorldMatrix(transform); });
}

Mat4 TransformSystem::composeWorldMatrix(const components::TransformComponent& component)
{
    return toLegacyTransform(component).modelMatrix();
}

Transform TransformSystem::toLegacyTransform(const components::TransformComponent& component)
{
    Transform transform{};
    transform.position = component.position;
    transform.rotation = component.rotation;
    transform.scale = component.scale;
    return transform;
}
} // namespace engine::systems
