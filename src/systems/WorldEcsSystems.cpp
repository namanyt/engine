#include "systems/WorldEcsSystems.h"

#include "components/WorldComponents.h"
#include "systems/RenderSystem.h"
#include "systems/TransformSystem.h"
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
    if (kind == WorldObjectKind::Terrain)
    {
        registry.emplace<components::TerrainComponent>(entity);
    }

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
    TransformSystem::updateWorldTransforms(scene);
    syncLegacySceneFromRenderView(scene, buildRenderSceneView(scene));
}

void syncPlayerEntity(Scene& scene, ecs::Entity entity, const Player& player, bool activeCamera)
{
    ecs::Registry& registry = scene.registry();
    if (!registry.isAlive(entity))
    {
        return;
    }

    if (!registry.has<components::NameComponent>(entity))
    {
        registry.emplace<components::NameComponent>(entity, std::string{"Player"});
    }

    if (!registry.has<components::TransformComponent>(entity))
    {
        registry.emplace<components::TransformComponent>(
            entity, player.position(), Vec3{0.0f, player.camera().yawDegrees(), 0.0f},
            Vec3{1.0f, 1.0f, 1.0f});
    }

    if (!registry.has<components::VelocityComponent>(entity))
    {
        registry.emplace<components::VelocityComponent>(entity, player.velocity());
    }

    if (!registry.has<components::ColliderComponent>(entity))
    {
        registry.emplace<components::ColliderComponent>(entity, player.collisionRadius(),
                                                        player.collisionHeight());
    }

    if (!registry.has<components::PlayerComponent>(entity))
    {
        registry.emplace<components::PlayerComponent>(entity, true);
    }
    else
    {
        registry.get<components::PlayerComponent>(entity).active = true;
    }

    if (!registry.has<components::PlayerInputComponent>(entity))
    {
        registry.emplace<components::PlayerInputComponent>(entity);
    }

    if (!registry.has<components::GroundingComponent>(entity))
    {
        registry.emplace<components::GroundingComponent>(entity);
    }

    if (!registry.has<components::CameraPresentationComponent>(entity))
    {
        registry.emplace<components::CameraPresentationComponent>(entity);
    }

    if (!registry.has<components::PlayerControllerComponent>(entity))
    {
        registry.emplace<components::PlayerControllerComponent>(entity);
    }

    if (!registry.has<components::CameraComponent>(entity))
    {
        registry.emplace<components::CameraComponent>(
            entity, player.eyeHeight(), activeCamera, false, player.camera().yawDegrees(),
            player.camera().pitchDegrees(), player.camera().rollDegrees(),
            player.camera().fieldOfViewRadians(), player.camera().nearPlane());
    }
    else
    {
        components::CameraComponent& camera = registry.get<components::CameraComponent>(entity);
        camera.active = activeCamera;
        camera.debugFreeCamera = false;
    }
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
