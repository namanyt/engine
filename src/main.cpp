#include "Application.h"

#include "core/Log.h"
#include "core/RenderDebug.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include "core/ShaderLibrary.h"
#include "ecs/Entity.h"
#include "primitives/Cone.h"
#include "primitives/Cube.h"
#include "primitives/Plane.h"
#include "primitives/Pyramid.h"
#include "primitives/Sphere.h"
#include "primitives/Cylinder.h"
#include "systems/WorldEcsSystems.h"
#include "world/Camera.h"
#include "world/FreeCameraController.h"
#include "world/Player.h"
#include "world/PlayerController.h"
#include "world/TestWorld.h"
#include "world/WorldNavigation.h"

#include <cstdlib>
#include <exception>
#include <memory>
#include <sstream>

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
#include "debug/DebugUi.h"
#endif

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

void copyCameraPose(const engine::Camera& source, engine::Camera& destination)
{
    destination.setPosition(source.position());
    destination.setYawPitch(source.yawDegrees(), source.pitchDegrees());
}
} // namespace

int main()
{
    try
    {
        engine::Application application;
        engine::Renderer renderer(application.shaderDirectory());
#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
        auto debugUi = std::make_unique<engine::DebugUi>(application.nativeWindow());
#endif
        engine::Camera debugCamera(engine::Vec3{0.0f, 1.8f, 12.0f});
        engine::FreeCameraController debugCameraController;
        engine::Player player;
        engine::PlayerController playerController;
        bool debugFreeCameraEnabled = false;
        engine::ecs::Entity playerEntity = engine::ecs::kInvalidEntity;
        engine::ecs::Entity debugCameraEntity = engine::ecs::kInvalidEntity;
        const engine::Shader& shader = renderer.shaderLibrary().loadGraphicsProgram(
            "surface.forward", "vertex.glsl", "fragment.glsl");
        engine::Plane plane;
        engine::Cube cube;
        engine::Pyramid pyramid;
        engine::Sphere sphere;
        engine::Cylinder cylinder;
        engine::Cone cone;
        const engine::TestWorldAssets worldAssets{
            &plane.mesh(),  &cube.mesh(), &cylinder.mesh(), &pyramid.mesh(),
            &sphere.mesh(), &cone.mesh(), &shader,
        };
        engine::Scene scene = engine::createAtmosphericTestWorld(worldAssets);

        const engine::Vec3 playerSpawn{
            0.0f,
            engine::sampleAtmosphericTerrainHeight(scene, 0.0f, 18.0f),
            18.0f,
        };
        player.setPosition(playerSpawn);
        player.setYawPitch(-90.0f, -4.5f);
        copyCameraPose(player.camera(), debugCamera);
        debugCameraController.setMoveSpeed(10.5f);
        debugCameraController.setSprintMultiplier(2.1f);

        auto ensureRuntimeEntities = [&]()
        {
            if (!scene.registry().isAlive(playerEntity))
            {
                playerEntity = scene.registry().createEntity();
            }

            if (!scene.registry().isAlive(debugCameraEntity))
            {
                debugCameraEntity = scene.registry().createEntity();
            }
        };

        ensureRuntimeEntities();
        engine::systems::syncPlayerEntity(scene, playerEntity, player, true);
        engine::systems::syncCameraEntity(scene, debugCameraEntity, debugCamera, false, true);

        {
            std::ostringstream stream;
            stream << "Loaded atmospheric scene with " << scene.objects().size()
                   << " world objects.";
            engine::Log::info("Main", stream.str());
        }

        application.pollEvents();
        static_cast<void>(application.consumeInputState());
        application.primeFrameState();
        engine::Log::info("Main", "Entering world loop.");

        engine::Mat4 previousViewProjectionMatrix = engine::Mat4::identity();
        engine::Mat4 previousInverseProjectionMatrix = engine::Mat4::identity();
        engine::Vec3 previousViewPosition{};
        engine::Vec3 previousViewForward{0.0f, 0.0f, -1.0f};
        engine::Vec3 previousLightDirection = scene.sunLight.direction;
        bool hasPreviousViewProjection = false;
        int frameIndex = 0;

        while (application.isRunning())
        {
            renderer.profiler().beginFrame();
            application.pollEvents();
            application.processInput();
            const engine::InputState inputState = application.consumeInputState();
            const float deltaSeconds = application.deltaSeconds();
            const float timeSeconds = application.timeSeconds();

            {
                const auto terrainGenerationCpu =
                    renderer.profiler().makeCpuScope("Terrain Generation");
                engine::syncAtmosphericTestWorld(scene, worldAssets);
            }
            ensureRuntimeEntities();

            if (inputState.toggleDebugFreeCamera)
            {
                debugFreeCameraEnabled = !debugFreeCameraEnabled;
                if (debugFreeCameraEnabled)
                {
                    copyCameraPose(player.camera(), debugCamera);
                }
            }

            if (debugFreeCameraEnabled)
            {
                debugCameraController.update(debugCamera, inputState, deltaSeconds);
            }
            else
            {
                playerController.update(player, scene, inputState, deltaSeconds);
            }

            {
                const auto ecsUpdateCpu = renderer.profiler().makeCpuScope("ECS Update");
                engine::systems::syncPlayerEntity(scene, playerEntity, player,
                                                  !debugFreeCameraEnabled);
                engine::systems::syncCameraEntity(scene, debugCameraEntity, debugCamera,
                                                  debugFreeCameraEnabled, true);
            }

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
            if (inputState.toggleDebugUi)
            {
                const bool debugUiEnabled = !debugUi->isEnabled();
                debugUi->setEnabled(debugUiEnabled);
                application.setCursorCaptured(!debugUiEnabled);
            }

            if (inputState.toggleCursorCapture && debugUi->isEnabled() &&
                !application.isCursorCaptured())
            {
                application.setCursorCaptured(true);
                debugUi->setEnabled(false);
            }
#endif

            if (inputState.toggleMoonLight)
            {
                engine::setMoonLightEnabled(scene, !engine::isMoonLightEnabled(scene));
            }

            if (inputState.toggleSphereLights)
            {
                engine::setSphereLightsEnabled(scene, !engine::areSphereLightsEnabled(scene));
            }

            if (inputState.toggleConeLights)
            {
                engine::setConeLightsEnabled(scene, !engine::areConeLightsEnabled(scene));
            }

            if (inputState.toggleMoonEmissive)
            {
                engine::setMoonEmissiveEnabled(scene, !engine::isMoonEmissiveEnabled(scene));
            }

            if (inputState.toggleSphereEmissive)
            {
                engine::setSphereEmissiveEnabled(scene, !engine::areSphereEmissiveEnabled(scene));
            }

            if (inputState.toggleConeEmissive)
            {
                engine::setConeEmissiveEnabled(scene, !engine::areConeEmissiveEnabled(scene));
            }

            if (inputState.stepMoonBackward)
            {
                engine::stepMoonTime(scene, -8.0f);
            }

            if (inputState.stepMoonForward)
            {
                engine::stepMoonTime(scene, 8.0f);
            }

            if (inputState.toggleMoonMotion)
            {
                engine::setMoonMotionEnabled(scene, !engine::isMoonMotionEnabled(scene));
            }

            {
                const auto atmosphereUpdateCpu =
                    renderer.profiler().makeCpuScope("Atmosphere Update");
                engine::updateAtmosphericWorldLighting(scene, timeSeconds);
            }
            application.updateWindowTitle(timeSeconds);

            const engine::Camera& activeCamera =
                debugFreeCameraEnabled ? debugCamera : player.camera();
            engine::syncAtmosphericMoonVisual(scene, activeCamera.position());

            renderer.setViewport(application.framebufferWidth(), application.framebufferHeight());

            const float aspectRatio = static_cast<float>(application.framebufferWidth()) /
                                      static_cast<float>(application.framebufferHeight() > 0
                                                             ? application.framebufferHeight()
                                                             : 1);
            const engine::Mat4 viewMatrix = activeCamera.viewMatrix();
            const engine::Mat4 projectionMatrix =
                makeCameraProjectionMatrix(activeCamera, aspectRatio);
            const engine::Mat4 viewProjectionMatrix = projectionMatrix * viewMatrix;
            engine::FrameUniforms frameUniforms{};
            frameUniforms.viewMatrix = viewMatrix;
            frameUniforms.projectionMatrix = projectionMatrix;
            frameUniforms.inverseViewMatrix = engine::inverse(viewMatrix);
            frameUniforms.inverseProjectionMatrix = engine::inverse(projectionMatrix);
            frameUniforms.previousInverseProjectionMatrix =
                hasPreviousViewProjection ? previousInverseProjectionMatrix
                                          : frameUniforms.inverseProjectionMatrix;
            frameUniforms.viewProjectionMatrix = viewProjectionMatrix;
            frameUniforms.previousViewProjectionMatrix =
                hasPreviousViewProjection ? previousViewProjectionMatrix : viewProjectionMatrix;
            frameUniforms.viewPosition = activeCamera.position();
            frameUniforms.previousViewPosition =
                hasPreviousViewProjection ? previousViewPosition : activeCamera.position();
            frameUniforms.viewForward = activeCamera.front();
            frameUniforms.previousViewForward =
                hasPreviousViewProjection ? previousViewForward : activeCamera.front();
            frameUniforms.viewRight = activeCamera.right();
            frameUniforms.viewUp = activeCamera.up();
            frameUniforms.timeSeconds = timeSeconds;
            frameUniforms.frameIndex = frameIndex;
            frameUniforms.aspectRatio = aspectRatio;
            frameUniforms.verticalFieldOfViewRadians = activeCamera.fieldOfViewRadians();
            frameUniforms.nearPlane = activeCamera.nearPlane();
            frameUniforms.fogColor = scene.fog.color;
            frameUniforms.fogDensity = scene.fog.density;
            frameUniforms.fogBaseHeight = scene.fog.baseHeight;
            frameUniforms.fogHeightFalloff = scene.fog.heightFalloff;
            frameUniforms.fogMaxHeight = scene.fog.maxHeight;
            frameUniforms.directionalLight = scene.sunLight;
            frameUniforms.previousLightDirection =
                hasPreviousViewProjection ? previousLightDirection : scene.sunLight.direction;
            frameUniforms.localLights = scene.localLights;
            frameUniforms.shadowSettings = scene.shadow;
            frameUniforms.skyLight = scene.skyLight;
            frameUniforms.rayEvaluation = scene.rayEvaluation;
            frameUniforms.debugView = scene.debugView;
            frameUniforms.rayTracingScene = scene.rayTracingScene;
            frameUniforms.exposure = scene.postProcess.exposure;
            frameUniforms.bloomThreshold = scene.postProcess.bloomThreshold;
            frameUniforms.lightViewProjectionMatrix = makeDirectionalLightViewProjection(scene);

            {
                const auto frameGpuScope = renderer.profiler().makeGpuScope("Frame");

                {
                    const auto shadowCpuScope =
                        renderer.profiler().makeCpuScope("Shadow Rendering");
                    const auto shadowGpuScope = renderer.profiler().makeGpuScope("Shadow Pass");
                    renderer.beginShadowPass(frameUniforms);

                    for (const engine::WorldObject& object : scene.objects())
                    {
                        if (object.mesh == nullptr || !object.castsShadows)
                        {
                            continue;
                        }

                        renderer.drawShadow(*object.mesh, object.transform);
                    }

                    renderer.endShadowPass();
                }

                renderer.beginFrame(scene.clearColor);

                {
                    engine::ScopedRenderDebugGroup terrainGroup("Terrain Pass");
                    const auto terrainGpuScope = renderer.profiler().makeGpuScope("Terrain Pass");
                    for (const engine::WorldObject& object : scene.objects())
                    {
                        if (object.kind != engine::WorldObjectKind::Terrain ||
                            object.mesh == nullptr || object.material.shader == nullptr)
                        {
                            continue;
                        }

                        renderer.draw(*object.mesh, object.material, object.transform,
                                      frameUniforms);
                    }
                }

                {
                    engine::ScopedRenderDebugGroup geometryGroup("Geometry Pass");
                    const auto geometryGpuScope = renderer.profiler().makeGpuScope("Geometry Pass");
                    for (const engine::WorldObject& object : scene.objects())
                    {
                        if (object.kind == engine::WorldObjectKind::Terrain ||
                            object.mesh == nullptr || object.material.shader == nullptr)
                        {
                            continue;
                        }

                        renderer.draw(*object.mesh, object.material, object.transform,
                                      frameUniforms);
                    }
                }

                renderer.endFrame(scene.postProcess, frameUniforms, timeSeconds);

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
                {
                    engine::ScopedRenderDebugGroup uiGroup("UI Pass");
                    const auto uiGpuScope = renderer.profiler().makeGpuScope("UI Pass");
                    debugUi->beginFrame();
                    const bool previousDebugFreeCameraEnabled = debugFreeCameraEnabled;
                    debugUi->draw(scene, player, playerController, debugFreeCameraEnabled,
                                  renderer.profiler().stats(), renderer.debugTextures());
                    if (debugFreeCameraEnabled && !previousDebugFreeCameraEnabled)
                    {
                        copyCameraPose(player.camera(), debugCamera);
                    }
                    debugUi->endFrame();
                }
#endif
            }

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
            if (debugUi->shouldQuit())
            {
                application.requestQuit();
            }

            if (debugUi->consumeResumeCameraRequest())
            {
                application.setCursorCaptured(true);
                debugUi->setEnabled(false);
            }
#endif

            renderer.profiler().endFrame();

            previousViewProjectionMatrix = viewProjectionMatrix;
            previousInverseProjectionMatrix = frameUniforms.inverseProjectionMatrix;
            previousViewPosition = frameUniforms.viewPosition;
            previousViewForward = frameUniforms.viewForward;
            previousLightDirection = frameUniforms.directionalLight.direction;
            hasPreviousViewProjection = true;
            ++frameIndex;

            application.present();
        }
    }
    catch (const std::exception& exception)
    {
        std::ostringstream stream;
        stream << "Fatal error: " << exception.what();
        engine::Log::error("Main", stream.str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
