#include "Application.h"

#include "core/Log.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include "core/ShaderLibrary.h"
#include "primitives/Cone.h"
#include "primitives/Cube.h"
#include "primitives/Plane.h"
#include "primitives/Pyramid.h"
#include "primitives/Sphere.h"
#include "primitives/Cylinder.h"
#include "world/Camera.h"
#include "world/FreeCameraController.h"
#include "world/TestWorld.h"

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
        engine::Camera camera(engine::Vec3{0.0f, 1.8f, 12.0f});
        engine::FreeCameraController cameraController;
        const engine::Shader& shader = renderer.shaderLibrary().loadGraphicsProgram(
            "surface.forward", "vertex.glsl", "fragment.glsl");
        engine::Plane plane;
        engine::Cube cube;
        engine::Pyramid pyramid;
        engine::Sphere sphere;
        engine::Cylinder cylinder;
        engine::Cone cone;
        engine::Scene scene = engine::createAtmosphericTestWorld(engine::TestWorldAssets{
            &plane.mesh(),
            &cube.mesh(),
            &cylinder.mesh(),
            &pyramid.mesh(),
            &sphere.mesh(),
            &cone.mesh(),
            &shader,
        });

        camera.setYawPitch(-90.0f, -4.5f);
        cameraController.setMoveSpeed(10.5f);
        cameraController.setSprintMultiplier(2.1f);

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

        while (application.isRunning())
        {
            application.pollEvents();
            application.processInput();
            const engine::InputState inputState = application.consumeInputState();
            const float deltaSeconds = application.deltaSeconds();
            const float timeSeconds = application.timeSeconds();
            cameraController.update(camera, inputState, deltaSeconds);

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

            engine::updateAtmosphericWorldLighting(scene, timeSeconds);
            application.updateWindowTitle(timeSeconds);

            renderer.setViewport(application.framebufferWidth(), application.framebufferHeight());

            const float aspectRatio = static_cast<float>(application.framebufferWidth()) /
                                      static_cast<float>(application.framebufferHeight() > 0
                                                             ? application.framebufferHeight()
                                                             : 1);
            engine::FrameUniforms frameUniforms{};
            frameUniforms.viewMatrix = camera.viewMatrix();
            frameUniforms.projectionMatrix = makeCameraProjectionMatrix(camera, aspectRatio);
            frameUniforms.viewPosition = camera.position();
            frameUniforms.viewForward = camera.front();
            frameUniforms.viewRight = camera.right();
            frameUniforms.viewUp = camera.up();
            frameUniforms.timeSeconds = timeSeconds;
            frameUniforms.aspectRatio = aspectRatio;
            frameUniforms.verticalFieldOfViewRadians = camera.fieldOfViewRadians();
            frameUniforms.nearPlane = camera.nearPlane();
            frameUniforms.fogColor = scene.fog.color;
            frameUniforms.fogDensity = scene.fog.density;
            frameUniforms.fogBaseHeight = scene.fog.baseHeight;
            frameUniforms.fogHeightFalloff = scene.fog.heightFalloff;
            frameUniforms.directionalLight = scene.sunLight;
            frameUniforms.localLights = scene.localLights;
            frameUniforms.shadowSettings = scene.shadow;
            frameUniforms.skyLight = scene.skyLight;
            frameUniforms.rayEvaluation = scene.rayEvaluation;
            frameUniforms.debugView = scene.debugView;
            frameUniforms.rayTracingScene = scene.rayTracingScene;
            frameUniforms.exposure = scene.postProcess.exposure;
            frameUniforms.bloomThreshold = scene.postProcess.bloomThreshold;
            frameUniforms.lightViewProjectionMatrix = makeDirectionalLightViewProjection(scene);

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
            renderer.beginFrame(scene.clearColor);

            for (const engine::WorldObject& object : scene.objects())
            {
                if (object.mesh == nullptr || object.material.shader == nullptr)
                {
                    continue;
                }

                renderer.draw(*object.mesh, object.material, object.transform, frameUniforms);
            }

            renderer.endFrame(scene.postProcess, frameUniforms, timeSeconds);

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
            debugUi->beginFrame();
            debugUi->draw(scene);
            debugUi->endFrame();

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
