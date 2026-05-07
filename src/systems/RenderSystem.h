#pragma once

#include "core/Renderer.h"
#include "math/Transform.h"
#include "world/Lighting.h"
#include "world/Material.h"
#include "world/RayTracing.h"
#include "world/Scene.h"

#include <vector>

namespace engine
{
class Camera;
class Mesh;
} // namespace engine

namespace engine::systems
{
struct FrameHistory final
{
    Mat4 previousViewProjectionMatrix = Mat4::identity();
    Mat4 previousInverseProjectionMatrix = Mat4::identity();
    Vec3 previousViewPosition{};
    Vec3 previousViewForward{0.0f, 0.0f, -1.0f};
    Vec3 previousLightDirection{0.0f, -1.0f, 0.0f};
    bool valid = false;
    int frameIndex = 0;
};

struct RenderItem final
{
    std::string debugName;
    WorldObjectId id = WorldObjectId::None;
    WorldObjectKind kind = WorldObjectKind::Monolith;
    unsigned int semantics = toSemanticFlags(WorldObjectSemantic::None);
    const Mesh* mesh = nullptr;
    Transform transform{};
    Mat4 modelMatrix = Mat4::identity();
    Material material{};
    bool castsShadows = true;
    bool visible = true;
};

struct RenderSceneView final
{
    std::vector<RenderItem> terrainItems;
    std::vector<RenderItem> geometryItems;
    std::vector<RenderItem> shadowCasters;
    std::vector<LocalLight> localLights;
    RayTracingScene rayTracingScene{};

    std::size_t renderableCount() const noexcept;
};

RenderSceneView buildRenderSceneView(const Scene& scene);
void syncLegacySceneFromRenderView(Scene& scene, const RenderSceneView& renderSceneView);
FrameUniforms buildFrameUniforms(const Scene& scene, const Camera& camera, int framebufferWidth,
                                 int framebufferHeight, float timeSeconds,
                                 const FrameHistory& frameHistory,
                                 const RenderSceneView& renderSceneView);
void advanceFrameHistory(const FrameUniforms& frameUniforms, FrameHistory& frameHistory);
} // namespace engine::systems
