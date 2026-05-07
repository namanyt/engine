#include "systems/WorldEcsSystems.h"

#include "components/WorldComponents.h"
#include "world/Camera.h"
#include "world/Player.h"
#include "world/Scene.h"

#include <string>
#include <utility>

namespace
{
template <typename Component, typename... Args>
Component& emplaceOrReplace(engine::ecs::Registry& registry, engine::ecs::Entity entity,
                            Args&&... args)
{
    if (registry.has<Component>(entity))
    {
        registry.remove<Component>(entity);
    }

    return registry.emplace<Component>(entity, std::forward<Args>(args)...);
}

engine::Transform buildLegacyTransform(const engine::components::TransformComponent& component)
{
    engine::Transform transform{};
    transform.position = component.position;
    transform.rotation = component.rotation;
    transform.scale = component.scale;
    return transform;
}
} // namespace

namespace engine::systems
{
ecs::Entity spawnWorldObjectEntity(Scene& scene, std::string_view debugName, WorldObjectId id,
                                   WorldObjectKind kind, unsigned int semantics, const Mesh* mesh,
                                   const Material& material, const Vec3& position,
                                   const Vec3& rotation, const Vec3& scale, bool castsShadows,
                                   float rayOccluderRadius, const Vec3& rayOccluderOffset)
{
    ecs::Registry& registry = scene.registry();
    const ecs::Entity entity = registry.createEntity();
    registry.emplace<components::NameComponent>(entity, std::string{debugName});
    registry.emplace<components::TransformComponent>(entity, position, rotation, scale);
    registry.emplace<components::RenderMeshComponent>(entity, mesh);
    registry.emplace<components::MaterialComponent>(entity, material);
    registry.emplace<components::WorldObjectComponent>(entity, id, kind, semantics, castsShadows);
    if (rayOccluderRadius > 0.0f)
    {
        registry.emplace<components::RayOccluderComponent>(entity, rayOccluderOffset,
                                                           rayOccluderRadius);
    }

    return entity;
}

ecs::Entity spawnLocalLightEntity(Scene& scene, std::string_view debugName, const Vec3& position,
                                  const Vec3& color, float intensity, float range,
                                  LocalLightGroup group, bool enabled, bool followMoon)
{
    ecs::Registry& registry = scene.registry();
    const ecs::Entity entity = registry.createEntity();
    registry.emplace<components::NameComponent>(entity, std::string{debugName});
    registry.emplace<components::TransformComponent>(entity, position, Vec3{},
                                                     Vec3{1.0f, 1.0f, 1.0f});
    registry.emplace<components::LocalLightComponent>(
        entity,
        LocalLight{position, color, intensity, range, intensity, enabled, followMoon, group});
    return entity;
}

ecs::Entity spawnRayOccluderEntity(Scene& scene, std::string_view debugName, const Vec3& position,
                                   float radius, const Vec3& centerOffset)
{
    ecs::Registry& registry = scene.registry();
    const ecs::Entity entity = registry.createEntity();
    registry.emplace<components::NameComponent>(entity, std::string{debugName});
    registry.emplace<components::TransformComponent>(entity, position, Vec3{},
                                                     Vec3{1.0f, 1.0f, 1.0f});
    registry.emplace<components::RayOccluderComponent>(entity, centerOffset, radius);
    return entity;
}

ecs::Entity findWorldObjectEntity(const Scene& scene, WorldObjectId id)
{
    ecs::Entity match = ecs::kInvalidEntity;
    scene.registry().forEach<components::WorldObjectComponent>(
        [&](ecs::Entity entity, const components::WorldObjectComponent& worldObject)
        {
            if (worldObject.id == id)
            {
                match = entity;
            }
        });
    return match;
}

void syncLegacySceneFromEcs(Scene& scene)
{
    scene.clearRuntimeViews();

    scene.registry()
        .forEach<components::TransformComponent, components::RenderMeshComponent,
                 components::MaterialComponent, components::WorldObjectComponent>(
            [&](ecs::Entity entity, const components::TransformComponent& transformComponent,
                const components::RenderMeshComponent& renderMesh,
                const components::MaterialComponent& materialComponent,
                const components::WorldObjectComponent& worldObjectComponent)
            {
                WorldObject object{};
                if (const components::NameComponent* name =
                        scene.registry().tryGet<components::NameComponent>(entity);
                    name != nullptr)
                {
                    object.debugName = name->value;
                }

                object.id = worldObjectComponent.id;
                object.kind = worldObjectComponent.kind;
                object.semantics = worldObjectComponent.semantics;
                object.mesh = renderMesh.mesh;
                object.transform = buildLegacyTransform(transformComponent);
                object.material = materialComponent.material;
                object.castsShadows = worldObjectComponent.castsShadows;
                scene.addObject(std::move(object));
            });

    scene.registry().forEach<components::TransformComponent, components::LocalLightComponent>(
        [&](ecs::Entity, const components::TransformComponent& transformComponent,
            const components::LocalLightComponent& lightComponent)
        {
            LocalLight light = lightComponent.light;
            light.position = transformComponent.position;
            scene.localLights.push_back(light);
        });

    scene.registry().forEach<components::TransformComponent, components::RayOccluderComponent>(
        [&](ecs::Entity, const components::TransformComponent& transformComponent,
            const components::RayOccluderComponent& rayOccluder)
        {
            scene.rayTracingScene.bounds.push_back(BoundingSphere{
                transformComponent.position + rayOccluder.centerOffset, rayOccluder.radius});
        });
}

void syncPlayerEntity(Scene& scene, ecs::Entity entity, const Player& player, bool activeCamera)
{
    ecs::Registry& registry = scene.registry();
    if (!registry.isAlive(entity))
    {
        return;
    }

    emplaceOrReplace<components::NameComponent>(registry, entity, std::string{"Player"});
    emplaceOrReplace<components::TransformComponent>(registry, entity, player.position(),
                                                     Vec3{0.0f, player.camera().yawDegrees(), 0.0f},
                                                     Vec3{1.0f, 1.0f, 1.0f});
    emplaceOrReplace<components::VelocityComponent>(registry, entity, player.velocity());
    emplaceOrReplace<components::ColliderComponent>(registry, entity, player.collisionRadius(),
                                                    player.collisionHeight());
    emplaceOrReplace<components::PlayerComponent>(registry, entity, true);
    emplaceOrReplace<components::CameraComponent>(
        registry, entity, player.eyeHeight(), activeCamera, false, player.camera().yawDegrees(),
        player.camera().pitchDegrees(), player.camera().fieldOfViewRadians(),
        player.camera().nearPlane());
}

void syncCameraEntity(Scene& scene, ecs::Entity entity, const Camera& camera, bool activeCamera,
                      bool debugFreeCamera)
{
    ecs::Registry& registry = scene.registry();
    if (!registry.isAlive(entity))
    {
        return;
    }

    emplaceOrReplace<components::NameComponent>(registry, entity, std::string{"DebugCamera"});
    emplaceOrReplace<components::TransformComponent>(
        registry, entity, camera.position(), Vec3{camera.pitchDegrees(), camera.yawDegrees(), 0.0f},
        Vec3{1.0f, 1.0f, 1.0f});
    emplaceOrReplace<components::CameraComponent>(
        registry, entity, 0.0f, activeCamera, debugFreeCamera, camera.yawDegrees(),
        camera.pitchDegrees(), camera.fieldOfViewRadians(), camera.nearPlane());
}
} // namespace engine::systems
