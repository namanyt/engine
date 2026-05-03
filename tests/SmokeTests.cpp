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

    return hasVertexShader && hasFragmentShader ? 0 : 1;
}

int dependencyLayoutExists()
{
    const std::filesystem::path root = repoRoot();

    const bool hasGlfw = requirePath(root / "external" / "glfw" / "CMakeLists.txt");
    const bool hasGladHeader = requirePath(root / "external" / "glad" / "include" / "glad" / "glad.h");
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
        root / "src" / "Renderer.h",
        root / "src" / "Renderer.cpp",
        root / "src" / "Shader.h",
        root / "src" / "Shader.cpp",
        root / "CMakeLists.txt"
    };

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
        {"engine_source_layout_exists", &engineSourceLayoutExists}
    };

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
