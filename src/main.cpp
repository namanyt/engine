#include "Application.h"

#include "core/Renderer.h"
#include "core/Shader.h"
#include "math/Transform.h"
#include "primitives/Cube.h"
#include "primitives/Quad.h"
#include "primitives/Sphere.h"

#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>

namespace
{
struct RenderObject final
{
    const engine::Mesh* mesh = nullptr;
    engine::Transform transform{};
    float timeOffset = 0.0f;
    float animationType = 0.0f;
};
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
        engine::Cube cube;
        engine::Quad quad;
        engine::Sphere sphere;

        std::array<RenderObject, 3> sceneObjects = {};

        sceneObjects[0].mesh = &cube.mesh();
        sceneObjects[0].transform.position = engine::Vec3{-1.8f, 0.1f, 0.0f};
        sceneObjects[0].transform.rotation = engine::Vec3{0.25f, 0.35f, 0.0f};
        sceneObjects[0].timeOffset = 0.0f;
        sceneObjects[0].animationType = 0.0f;

        sceneObjects[1].mesh = &quad.mesh();
        sceneObjects[1].transform.position = engine::Vec3{0.0f, -0.3f, -0.1f};
        sceneObjects[1].transform.rotation = engine::Vec3{0.0f, 0.0f, 0.0f};
        sceneObjects[1].transform.scale = engine::Vec3{1.6f, 1.6f, 1.6f};
        sceneObjects[1].timeOffset = 1.2f;
        sceneObjects[1].animationType = 1.0f;

        sceneObjects[2].mesh = &sphere.mesh();
        sceneObjects[2].transform.position = engine::Vec3{1.8f, 0.0f, 0.0f};
        sceneObjects[2].transform.scale = engine::Vec3{1.15f, 1.15f, 1.15f};
        sceneObjects[2].timeOffset = 2.4f;
        sceneObjects[2].animationType = 2.0f;

        while (application.isRunning())
        {
            application.processInput();

            const float timeSeconds = application.timeSeconds();

            renderer.setViewport(application.framebufferWidth(), application.framebufferHeight());
            renderer.beginFrame();

            for (const RenderObject& object : sceneObjects)
            {
                renderer.draw(
                    *object.mesh,
                    shader,
                    object.transform,
                    timeSeconds,
                    object.animationType,
                    object.timeOffset);
            }

            application.present();
            application.pollEvents();
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
