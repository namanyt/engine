#include "systems/RenderSystem.h"

#include "components/WorldComponents.h"
#include "systems/TransformSystem.h"
#include "world/Camera.h"

#include <utility>

namespace
{
engine::Mat4 makeDirectionalLightViewProjection(const engine::Scene& scene)
{
    const engine::Vec3 lightDirection = engine::normalize(scene.sunLight.direction);
    const engine::Vec3 focusPoint = scene.shadow.focusPoint;
    const engine::Vec3 eye = focusPoint - lightDirection * (scene.shadow.projectionRadius * 0.8f);
    const engine::Mat4 lightView =
        engine::makeLookAt(eye, focusPoint, engine::Vec3{0.0f, 1.0f, 0.0f});
    const float radius = scene.shadow.projectionRadius;
    const engine::Mat4 lightProjection = engine::makeOrthographic(
        -radius, radius, -radius, radius, scene.shadow.nearPlane, scene.shadow.farPlane);
    return lightProjection * lightView;
}

engine::Mat4 makeCameraProjectionMatrix(const engine::Camera& camera, float aspectRatio)
{
    const float safeAspectRatio = aspectRatio > 0.01f ? aspectRatio : 0.01f;
    return engine::makeInfinitePerspective(camera.fieldOfViewRadians(), safeAspectRatio,
                                           camera.nearPlane());
}
} // namespace

namespace engine::systems
{
std::size_t RenderSceneView::renderableCount() const noexcept
{
    return terrainItems.size() + geometryItems.size();
}

RenderSceneView buildRenderSceneView(const Scene& scene)
{
    RenderSceneView renderSceneView{};

    scene.registry()
        .forEach<components::TransformComponent, components::RenderMeshComponent,
                 components::MaterialComponent, components::WorldObjectComponent>(
            [&](ecs::Entity entity, const components::TransformComponent& transformComponent,
                const components::RenderMeshComponent& renderMesh,
                const components::MaterialComponent& materialComponent,
                const components::WorldObjectComponent& worldObjectComponent)
            {
                if (!renderMesh.visible)
                {
                    return;
                }

                RenderItem item{};
                if (const components::NameComponent* name =
                        scene.registry().tryGet<components::NameComponent>(entity);
                    name != nullptr)
                {
                    item.debugName = name->value;
                }

                item.id = worldObjectComponent.id;
                item.kind = worldObjectComponent.kind;
                item.semantics = worldObjectComponent.semantics;
                item.mesh = renderMesh.mesh;
                item.transform = TransformSystem::toLegacyTransform(transformComponent);
                item.modelMatrix = transformComponent.worldMatrix;
                item.material = materialComponent.material;
                item.castsShadows = worldObjectComponent.castsShadows && renderMesh.castsShadows;
                item.visible = renderMesh.visible;

                if (item.kind == WorldObjectKind::Terrain)
                {
                    renderSceneView.terrainItems.push_back(item);
                }
                else
                {
                    renderSceneView.geometryItems.push_back(item);
                }

                if (item.castsShadows && item.mesh != nullptr)
                {
                    renderSceneView.shadowCasters.push_back(std::move(item));
                }
            });

    scene.registry().forEach<components::TransformComponent, components::LocalLightComponent>(
        [&](ecs::Entity, const components::TransformComponent& transformComponent,
            const components::LocalLightComponent& lightComponent)
        {
            LocalLight light = lightComponent.light;
            light.position = transformComponent.position;
            renderSceneView.localLights.push_back(light);
        });

    scene.registry().forEach<components::TransformComponent, components::RayOccluderComponent>(
        [&](ecs::Entity, const components::TransformComponent& transformComponent,
            const components::RayOccluderComponent& rayOccluder)
        {
            renderSceneView.rayTracingScene.bounds.push_back(BoundingSphere{
                transformComponent.position + rayOccluder.centerOffset, rayOccluder.radius});
        });

    return renderSceneView;
}

void syncLegacySceneFromRenderView(Scene& scene, const RenderSceneView& renderSceneView)
{
    scene.clearRuntimeViews();

    auto appendObjects = [&](const std::vector<RenderItem>& items)
    {
        for (const RenderItem& item : items)
        {
            WorldObject object{};
            object.debugName = item.debugName;
            object.id = item.id;
            object.kind = item.kind;
            object.semantics = item.semantics;
            object.mesh = item.mesh;
            object.transform = item.transform;
            object.material = item.material;
            object.castsShadows = item.castsShadows;
            scene.addObject(std::move(object));
        }
    };

    appendObjects(renderSceneView.terrainItems);
    appendObjects(renderSceneView.geometryItems);
    scene.localLights = renderSceneView.localLights;
    scene.rayTracingScene = renderSceneView.rayTracingScene;
}

FrameUniforms buildFrameUniforms(const Scene& scene, const Camera& camera, int framebufferWidth,
                                 int framebufferHeight, float timeSeconds,
                                 const FrameHistory& frameHistory,
                                 const RenderSceneView& renderSceneView)
{
    const int safeHeight = framebufferHeight > 0 ? framebufferHeight : 1;
    const float aspectRatio = static_cast<float>(framebufferWidth) / static_cast<float>(safeHeight);
    const Mat4 viewMatrix = camera.viewMatrix();
    const Mat4 projectionMatrix = makeCameraProjectionMatrix(camera, aspectRatio);
    const Mat4 viewProjectionMatrix = projectionMatrix * viewMatrix;

    FrameUniforms frameUniforms{};
    frameUniforms.viewMatrix = viewMatrix;
    frameUniforms.projectionMatrix = projectionMatrix;
    frameUniforms.inverseViewMatrix = inverse(viewMatrix);
    frameUniforms.inverseProjectionMatrix = inverse(projectionMatrix);
    frameUniforms.previousInverseProjectionMatrix =
        frameHistory.valid ? frameHistory.previousInverseProjectionMatrix
                           : frameUniforms.inverseProjectionMatrix;
    frameUniforms.viewProjectionMatrix = viewProjectionMatrix;
    frameUniforms.previousViewProjectionMatrix =
        frameHistory.valid ? frameHistory.previousViewProjectionMatrix : viewProjectionMatrix;
    frameUniforms.viewPosition = camera.position();
    frameUniforms.previousViewPosition =
        frameHistory.valid ? frameHistory.previousViewPosition : camera.position();
    frameUniforms.viewForward = camera.front();
    frameUniforms.previousViewForward =
        frameHistory.valid ? frameHistory.previousViewForward : camera.front();
    frameUniforms.viewRight = camera.right();
    frameUniforms.viewUp = camera.up();
    frameUniforms.timeSeconds = timeSeconds;
    frameUniforms.frameIndex = frameHistory.frameIndex;
    frameUniforms.aspectRatio = aspectRatio;
    frameUniforms.verticalFieldOfViewRadians = camera.fieldOfViewRadians();
    frameUniforms.nearPlane = camera.nearPlane();
    frameUniforms.fogColor = scene.fog.color;
    frameUniforms.fogDensity = scene.fog.density;
    frameUniforms.fogBaseHeight = scene.fog.baseHeight;
    frameUniforms.fogHeightFalloff = scene.fog.heightFalloff;
    frameUniforms.fogMaxHeight = scene.fog.maxHeight;
    frameUniforms.directionalLight = scene.sunLight;
    frameUniforms.previousLightDirection =
        frameHistory.valid ? frameHistory.previousLightDirection : scene.sunLight.direction;
    frameUniforms.localLights = renderSceneView.localLights;
    frameUniforms.shadowSettings = scene.shadow;
    frameUniforms.skyLight = scene.skyLight;
    frameUniforms.rayEvaluation = scene.rayEvaluation;
    frameUniforms.debugView = scene.debugView;
    frameUniforms.rayTracingScene = renderSceneView.rayTracingScene;
    frameUniforms.exposure = scene.postProcess.exposure;
    frameUniforms.bloomThreshold = scene.postProcess.bloomThreshold;
    frameUniforms.lightViewProjectionMatrix = makeDirectionalLightViewProjection(scene);
    return frameUniforms;
}

void advanceFrameHistory(const FrameUniforms& frameUniforms, FrameHistory& frameHistory)
{
    frameHistory.previousViewProjectionMatrix = frameUniforms.viewProjectionMatrix;
    frameHistory.previousInverseProjectionMatrix = frameUniforms.inverseProjectionMatrix;
    frameHistory.previousViewPosition = frameUniforms.viewPosition;
    frameHistory.previousViewForward = frameUniforms.viewForward;
    frameHistory.previousLightDirection = frameUniforms.directionalLight.direction;
    frameHistory.valid = true;
    ++frameHistory.frameIndex;
}
} // namespace engine::systems
