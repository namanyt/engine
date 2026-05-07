#include "Application.h"

#include "core/Log.h"
#include "core/RenderPipeline.h"
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
#include "systems/RenderSystem.h"
#include "systems/TransformSystem.h"
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
        engine::RenderPipeline renderPipeline(renderer);
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

        engine::systems::FrameHistory frameHistory{};

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
                playerController.update(player, scene, playerEntity, inputState, deltaSeconds);
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
                debugUi->setEnabled(!debugUi->isEnabled());
            }

            if (inputState.toggleCursorCapture)
            {
                application.setCursorCaptured(!application.isCursorCaptured());
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
            engine::systems::TransformSystem::updateWorldTransforms(scene);
            const engine::systems::RenderSceneView renderSceneView =
                engine::systems::buildRenderSceneView(scene);
            engine::systems::syncLegacySceneFromRenderView(scene, renderSceneView);
            const engine::FrameUniforms frameUniforms = engine::systems::buildFrameUniforms(
                scene, activeCamera, application.framebufferWidth(),
                application.framebufferHeight(), timeSeconds, frameHistory, renderSceneView);

            renderPipeline.renderFrame(renderSceneView, scene.clearColor, scene.postProcess,
                                       frameUniforms, timeSeconds);

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

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
            if (debugUi->shouldQuit())
            {
                application.requestQuit();
            }

            if (debugUi->consumeResumeCameraRequest())
            {
                application.setCursorCaptured(true);
            }
#endif

            renderer.profiler().endFrame();

            engine::systems::advanceFrameHistory(frameUniforms, frameHistory);

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
