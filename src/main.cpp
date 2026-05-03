#include "Application.h"

#include "core/Log.h"
#include "core/Renderer.h"
#include "core/Shader.h"
#include "math/Transform.h"
#include "primitives/Capsule.h"
#include "primitives/Cone.h"
#include "primitives/Cube.h"
#include "primitives/Cylinder.h"
#include "primitives/Plane.h"
#include "primitives/Pyramid.h"
#include "primitives/Quad.h"
#include "primitives/Sphere.h"
#include "primitives/Torus.h"
#include "primitives/Triangle.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <sstream>
#include <string_view>

namespace
{
struct RenderObject final
{
    std::string_view label;
    const engine::Mesh* mesh = nullptr;
    engine::Transform transform{};
    engine::Vec3 basePosition{};
    engine::Vec3 baseRotation{};
    engine::Vec3 baseScale{1.0f, 1.0f, 1.0f};
    float rotationSpeed = 0.0f;
    float bobAmplitude = 0.0f;
    float bobSpeed = 0.0f;
    float scaleAmplitude = 0.0f;
    float scaleSpeed = 0.0f;
    float phaseOffset = 0.0f;
};

constexpr float kPi = 3.14159265359f;
} // namespace

int main()
{
    try
    {
        engine::Application application;
        engine::Renderer renderer;
        engine::Shader shader(
            application.shaderDirectory() / "vertex.glsl",
            application.shaderDirectory() / "fragment.glsl");
        engine::Triangle triangle;
        engine::Plane plane;
        engine::Cube cube;
        engine::Quad quad;
        engine::Pyramid pyramid;
        engine::Sphere sphere;
        engine::Cylinder cylinder;
        engine::Cone cone;
        engine::Capsule capsule;
        engine::Torus torus;

        engine::Log::info("Main", "Building primitive gallery scene.");

        std::array<RenderObject, 10> sceneObjects = {};

        sceneObjects[0].label = "Triangle";
        sceneObjects[0].mesh = &triangle.mesh();
        sceneObjects[0].basePosition = engine::Vec3{-5.0f, 2.4f, 0.0f};
        sceneObjects[0].baseRotation = engine::Vec3{0.0f, 0.0f, -0.18f};
        sceneObjects[0].baseScale = engine::Vec3{0.95f, 0.95f, 0.95f};
        sceneObjects[0].rotationSpeed = 0.6f;
        sceneObjects[0].bobAmplitude = 0.18f;
        sceneObjects[0].bobSpeed = 1.2f;
        sceneObjects[0].phaseOffset = 0.0f;

        sceneObjects[1].label = "Quad";
        sceneObjects[1].mesh = &quad.mesh();
        sceneObjects[1].basePosition = engine::Vec3{-1.7f, 2.4f, 0.0f};
        sceneObjects[1].baseRotation = engine::Vec3{0.0f, 0.0f, 0.35f};
        sceneObjects[1].baseScale = engine::Vec3{1.05f, 1.05f, 1.05f};
        sceneObjects[1].rotationSpeed = -0.45f;
        sceneObjects[1].scaleAmplitude = 0.08f;
        sceneObjects[1].scaleSpeed = 1.4f;
        sceneObjects[1].phaseOffset = 0.5f;

        sceneObjects[2].label = "Plane";
        sceneObjects[2].mesh = &plane.mesh();
        sceneObjects[2].basePosition = engine::Vec3{1.7f, 2.4f, 0.0f};
        sceneObjects[2].baseRotation = engine::Vec3{-0.42f * kPi, 0.28f, 0.0f};
        sceneObjects[2].baseScale = engine::Vec3{1.25f, 1.25f, 1.25f};
        sceneObjects[2].rotationSpeed = 0.35f;
        sceneObjects[2].bobAmplitude = 0.12f;
        sceneObjects[2].bobSpeed = 0.9f;
        sceneObjects[2].phaseOffset = 1.0f;

        sceneObjects[3].label = "Cube";
        sceneObjects[3].mesh = &cube.mesh();
        sceneObjects[3].basePosition = engine::Vec3{5.0f, 2.4f, 0.0f};
        sceneObjects[3].baseRotation = engine::Vec3{0.45f, 0.65f, 0.0f};
        sceneObjects[3].baseScale = engine::Vec3{0.95f, 0.95f, 0.95f};
        sceneObjects[3].rotationSpeed = 1.1f;
        sceneObjects[3].phaseOffset = 1.6f;

        sceneObjects[4].label = "Pyramid";
        sceneObjects[4].mesh = &pyramid.mesh();
        sceneObjects[4].basePosition = engine::Vec3{-5.0f, -0.6f, 0.0f};
        sceneObjects[4].baseRotation = engine::Vec3{0.0f, -0.45f, 0.0f};
        sceneObjects[4].baseScale = engine::Vec3{0.95f, 1.15f, 0.95f};
        sceneObjects[4].rotationSpeed = -0.75f;
        sceneObjects[4].bobAmplitude = 0.15f;
        sceneObjects[4].bobSpeed = 1.5f;
        sceneObjects[4].phaseOffset = 2.0f;

        sceneObjects[5].label = "Sphere";
        sceneObjects[5].mesh = &sphere.mesh();
        sceneObjects[5].basePosition = engine::Vec3{-1.7f, -0.6f, 0.0f};
        sceneObjects[5].baseScale = engine::Vec3{0.95f, 0.95f, 0.95f};
        sceneObjects[5].scaleAmplitude = 0.10f;
        sceneObjects[5].scaleSpeed = 1.9f;
        sceneObjects[5].phaseOffset = 2.4f;

        sceneObjects[6].label = "Cylinder";
        sceneObjects[6].mesh = &cylinder.mesh();
        sceneObjects[6].basePosition = engine::Vec3{1.7f, -0.6f, 0.0f};
        sceneObjects[6].baseRotation = engine::Vec3{0.0f, 0.35f, 0.0f};
        sceneObjects[6].baseScale = engine::Vec3{0.85f, 1.15f, 0.85f};
        sceneObjects[6].rotationSpeed = 0.7f;
        sceneObjects[6].phaseOffset = 2.9f;

        sceneObjects[7].label = "Cone";
        sceneObjects[7].mesh = &cone.mesh();
        sceneObjects[7].basePosition = engine::Vec3{5.0f, -0.6f, 0.0f};
        sceneObjects[7].baseRotation = engine::Vec3{0.0f, -0.35f, 0.0f};
        sceneObjects[7].baseScale = engine::Vec3{0.95f, 1.05f, 0.95f};
        sceneObjects[7].rotationSpeed = -0.6f;
        sceneObjects[7].bobAmplitude = 0.14f;
        sceneObjects[7].bobSpeed = 1.1f;
        sceneObjects[7].phaseOffset = 3.3f;

        sceneObjects[8].label = "Capsule";
        sceneObjects[8].mesh = &capsule.mesh();
        sceneObjects[8].basePosition = engine::Vec3{-2.6f, -3.6f, 0.0f};
        sceneObjects[8].baseRotation = engine::Vec3{0.35f, 0.45f, 0.0f};
        sceneObjects[8].baseScale = engine::Vec3{0.9f, 1.2f, 0.9f};
        sceneObjects[8].rotationSpeed = 0.85f;
        sceneObjects[8].phaseOffset = 3.9f;

        sceneObjects[9].label = "Torus";
        sceneObjects[9].mesh = &torus.mesh();
        sceneObjects[9].basePosition = engine::Vec3{2.6f, -3.6f, 0.0f};
        sceneObjects[9].baseRotation = engine::Vec3{1.1f, 0.35f, 0.25f};
        sceneObjects[9].baseScale = engine::Vec3{1.05f, 1.05f, 1.05f};
        sceneObjects[9].rotationSpeed = -1.0f;
        sceneObjects[9].scaleAmplitude = 0.07f;
        sceneObjects[9].scaleSpeed = 1.6f;
        sceneObjects[9].phaseOffset = 4.4f;

        {
            std::ostringstream stream;
            stream << "Scene objects configured: " << sceneObjects.size();
            engine::Log::info("Main", stream.str());
        }

        for (const RenderObject& object : sceneObjects)
        {
            std::ostringstream stream;
            stream
                << object.label
                << " pos=("
                << object.basePosition.x
                << ", "
                << object.basePosition.y
                << ", "
                << object.basePosition.z
                << ") rotYSpeed="
                << object.rotationSpeed
                << " bobAmp="
                << object.bobAmplitude
                << " scaleAmp="
                << object.scaleAmplitude;
            engine::Log::info("Main", stream.str());
        }

        engine::Log::info("Main", "Entering main loop.");

        while (application.isRunning())
        {
            application.processInput();
            const float timeSeconds = application.timeSeconds();
            application.updateWindowTitle(timeSeconds);

            renderer.setViewport(application.framebufferWidth(), application.framebufferHeight());
            renderer.beginFrame();

            for (RenderObject& object : sceneObjects)
            {
                const float phaseTime = timeSeconds + object.phaseOffset;
                const float bob = std::sin(phaseTime * object.bobSpeed) * object.bobAmplitude;
                const float uniformScale = 1.0f + std::sin(phaseTime * object.scaleSpeed) * object.scaleAmplitude;

                object.transform.position = object.basePosition + engine::Vec3{0.0f, bob, 0.0f};
                object.transform.rotation = object.baseRotation + engine::Vec3{0.0f, timeSeconds * object.rotationSpeed, 0.0f};
                object.transform.scale = object.baseScale * uniformScale;

                renderer.draw(*object.mesh, shader, object.transform);
            }

            application.present();
            application.pollEvents();
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
