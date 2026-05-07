#pragma once

#include "ecs/Entity.h"
#include "math/Types.h"

#include <string_view>

namespace engine
{
class Camera;
class Mesh;
class Player;
class Scene;
struct Material;
enum class LocalLightGroup;
enum class WorldObjectId;
enum class WorldObjectKind;
} // namespace engine

namespace engine::systems
{
ecs::Entity spawnWorldObjectEntity(Scene& scene, std::string_view debugName, WorldObjectId id,
                                   WorldObjectKind kind, unsigned int semantics, const Mesh* mesh,
                                   const Material& material, const Vec3& position,
                                   const Vec3& rotation, const Vec3& scale,
                                   bool castsShadows = true, float rayOccluderRadius = 0.0f,
                                   const Vec3& rayOccluderOffset = Vec3{});

ecs::Entity spawnLocalLightEntity(Scene& scene, std::string_view debugName, const Vec3& position,
                                  const Vec3& color, float intensity, float range,
                                  LocalLightGroup group, bool enabled = true,
                                  bool followMoon = false);

ecs::Entity spawnRayOccluderEntity(Scene& scene, std::string_view debugName, const Vec3& position,
                                   float radius, const Vec3& centerOffset = Vec3{});

ecs::Entity findWorldObjectEntity(const Scene& scene, WorldObjectId id);
void syncLegacySceneFromEcs(Scene& scene);
void syncPlayerEntity(Scene& scene, ecs::Entity entity, const Player& player, bool activeCamera);
void syncCameraEntity(Scene& scene, ecs::Entity entity, const Camera& camera, bool activeCamera,
                      bool debugFreeCamera);
} // namespace engine::systems
