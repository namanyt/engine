#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
std::filesystem::path repoRoot()
{
#ifdef ENGINE_SOURCE_DIR
    return std::filesystem::path(ENGINE_SOURCE_DIR);
#else
    return std::filesystem::current_path();
#endif
}

bool requirePath(const std::filesystem::path& path)
{
    if (std::filesystem::exists(path))
    {
        return true;
    }

    std::cerr << "Missing required path: " << path.string() << '\n';
    return false;
}

int shaderAssetsExist()
{
    const std::filesystem::path root = repoRoot();

    const bool hasVertexShader = requirePath(root / "shaders" / "vertex.glsl");
    const bool hasFragmentShader = requirePath(root / "shaders" / "fragment.glsl");
    const bool hasPostBlurVertexShader = requirePath(root / "shaders" / "post_blur.vert");
    const bool hasPostBlurFragmentShader = requirePath(root / "shaders" / "post_blur.frag");
    const bool hasPostComposeFragmentShader = requirePath(root / "shaders" / "post_compose.frag");
    const bool hasRayEvalVertexShader = requirePath(root / "shaders" / "ray_eval.vert");
    const bool hasRayEvalFragmentShader = requirePath(root / "shaders" / "ray_eval.frag");
    const bool hasPostTonemapVertexShader = requirePath(root / "shaders" / "post_tonemap.vert");
    const bool hasPostTonemapFragmentShader = requirePath(root / "shaders" / "post_tonemap.frag");
    const bool hasShadowDepthVertexShader = requirePath(root / "shaders" / "shadow_depth.vert");
    const bool hasShadowDepthFragmentShader = requirePath(root / "shaders" / "shadow_depth.frag");

    return hasVertexShader && hasFragmentShader && hasPostBlurVertexShader &&
                   hasPostBlurFragmentShader && hasPostComposeFragmentShader &&
                   hasRayEvalVertexShader && hasRayEvalFragmentShader &&
                   hasPostTonemapVertexShader && hasPostTonemapFragmentShader &&
                   hasShadowDepthVertexShader && hasShadowDepthFragmentShader
               ? 0
               : 1;
}

int dependencyLayoutExists()
{
    const std::filesystem::path root = repoRoot();

    const bool hasGlfw = requirePath(root / "external" / "glfw" / "CMakeLists.txt");
    const bool hasGladHeader =
        requirePath(root / "external" / "glad" / "include" / "glad" / "glad.h");
    const bool hasGladSource = requirePath(root / "external" / "glad" / "src" / "glad.c");

    return hasGlfw && hasGladHeader && hasGladSource ? 0 : 1;
}

int engineSourceLayoutExists()
{
    const std::filesystem::path root = repoRoot();

    const std::vector<std::filesystem::path> requiredFiles = {
        root / "src" / "main.cpp",
        root / "src" / "Application.h",
        root / "src" / "Application.cpp",
        root / "src" / "core" / "Log.h",
        root / "src" / "core" / "Log.cpp",
        root / "src" / "core" / "Renderer.h",
        root / "src" / "core" / "FullScreenPass.h",
        root / "src" / "core" / "FullScreenPass.cpp",
        root / "src" / "core" / "PostProcessor.h",
        root / "src" / "core" / "PostProcessor.cpp",
        root / "src" / "core" / "RayEvaluationPass.h",
        root / "src" / "core" / "RayEvaluationPass.cpp",
        root / "src" / "core" / "ShadowMapPass.h",
        root / "src" / "core" / "ShadowMapPass.cpp",
        root / "src" / "debug" / "DebugUi.h",
        root / "src" / "debug" / "DebugUi.cpp",
        root / "src" / "core" / "Renderer.cpp",
        root / "src" / "core" / "Shader.h",
        root / "src" / "core" / "Shader.cpp",
        root / "src" / "geometry" / "Geometry.h",
        root / "src" / "geometry" / "Geometry.cpp",
        root / "src" / "geometry" / "Generators.h",
        root / "src" / "geometry" / "Generators.cpp",
        root / "src" / "geometry" / "Operations.h",
        root / "src" / "geometry" / "Operations.cpp",
        root / "src" / "graphics" / "VertexBuffer.h",
        root / "src" / "graphics" / "VertexBuffer.cpp",
        root / "src" / "graphics" / "IndexBuffer.h",
        root / "src" / "graphics" / "IndexBuffer.cpp",
        root / "src" / "graphics" / "VertexArray.h",
        root / "src" / "graphics" / "VertexArray.cpp",
        root / "src" / "graphics" / "Mesh.h",
        root / "src" / "graphics" / "Mesh.cpp",
        root / "src" / "primitives" / "PrimitiveBuilders.h",
        root / "src" / "primitives" / "PrimitiveBuilders.cpp",
        root / "src" / "primitives" / "Cube.h",
        root / "src" / "primitives" / "Cube.cpp",
        root / "src" / "primitives" / "Quad.h",
        root / "src" / "primitives" / "Quad.cpp",
        root / "src" / "primitives" / "Plane.h",
        root / "src" / "primitives" / "Plane.cpp",
        root / "src" / "primitives" / "Triangle.h",
        root / "src" / "primitives" / "Triangle.cpp",
        root / "src" / "primitives" / "Pyramid.h",
        root / "src" / "primitives" / "Pyramid.cpp",
        root / "src" / "primitives" / "Sphere.h",
        root / "src" / "primitives" / "Sphere.cpp",
        root / "src" / "primitives" / "Cylinder.h",
        root / "src" / "primitives" / "Cylinder.cpp",
        root / "src" / "primitives" / "Cone.h",
        root / "src" / "primitives" / "Cone.cpp",
        root / "src" / "primitives" / "Capsule.h",
        root / "src" / "primitives" / "Capsule.cpp",
        root / "src" / "primitives" / "Torus.h",
        root / "src" / "primitives" / "Torus.cpp",
        root / "src" / "math" / "Types.h",
        root / "src" / "math" / "Types.cpp",
        root / "src" / "math" / "Transform.h",
        root / "src" / "math" / "Transform.cpp",
        root / "src" / "world" / "Camera.h",
        root / "src" / "world" / "Camera.cpp",
        root / "src" / "world" / "FreeCameraController.h",
        root / "src" / "world" / "FreeCameraController.cpp",
        root / "src" / "world" / "Lighting.h",
        root / "src" / "world" / "Material.h",
        root / "src" / "world" / "RayTracing.h",
        root / "src" / "world" / "Scene.h",
        root / "src" / "world" / "TestWorld.h",
        root / "src" / "world" / "TestWorld.cpp",
        root / "CMakeLists.txt"};

    bool success = true;

    for (const auto& filePath : requiredFiles)
    {
        success = requirePath(filePath) && success;
    }

    return success ? 0 : 1;
}

struct NamedTest
{
    std::string_view name;
    int (*function)();
};
} // namespace

int main(int argc, char** argv)
{
    const std::vector<NamedTest> tests = {
        {"shader_assets_exist", &shaderAssetsExist},
        {"dependency_layout_exists", &dependencyLayoutExists},
        {"engine_source_layout_exists", &engineSourceLayoutExists}};

    if (argc != 2)
    {
        std::cerr << "Usage: EngineStarterTests <test-name>\n";
        return 1;
    }

    const std::string_view requestedTest = argv[1];

    for (const NamedTest& test : tests)
    {
        if (test.name == requestedTest)
        {
            return test.function();
        }
    }

    std::cerr << "Unknown test: " << requestedTest << '\n';
    return 1;
}
