#include "assets/AssetManager.h"
#include "assets/ShaderAsset.h"
#include "core/ShaderLibrary.h"
#include "runtime/VnScript.h"

#include <filesystem>
#include <fstream>
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

    const bool hasVertexShader = requirePath(root / "assets" / "shaders" / "vertex.glsl");
    const bool hasFragmentShader = requirePath(root / "assets" / "shaders" / "fragment.glsl");
    const bool hasPostBlurVertexShader =
        requirePath(root / "assets" / "shaders" / "post_blur.vert");
    const bool hasPostBlurFragmentShader =
        requirePath(root / "assets" / "shaders" / "post_blur.frag");
    const bool hasPostComposeFragmentShader =
        requirePath(root / "assets" / "shaders" / "post_compose.frag");
    const bool hasRayEvalVertexShader = requirePath(root / "assets" / "shaders" / "ray_eval.vert");
    const bool hasRayEvalFragmentShader =
        requirePath(root / "assets" / "shaders" / "ray_eval.frag");
    const bool hasPostTonemapVertexShader =
        requirePath(root / "assets" / "shaders" / "post_tonemap.vert");
    const bool hasPostTonemapFragmentShader =
        requirePath(root / "assets" / "shaders" / "post_tonemap.frag");
    const bool hasUiOverlayFragmentShader =
        requirePath(root / "assets" / "shaders" / "ui_overlay.frag");
    const bool hasShadowDepthVertexShader =
        requirePath(root / "assets" / "shaders" / "shadow_depth.vert");
    const bool hasShadowDepthFragmentShader =
        requirePath(root / "assets" / "shaders" / "shadow_depth.frag");

    return hasVertexShader && hasFragmentShader && hasPostBlurVertexShader &&
                   hasPostBlurFragmentShader && hasPostComposeFragmentShader &&
                   hasRayEvalVertexShader && hasRayEvalFragmentShader &&
                   hasPostTonemapVertexShader && hasPostTonemapFragmentShader &&
                   hasUiOverlayFragmentShader && hasShadowDepthVertexShader &&
                   hasShadowDepthFragmentShader
               ? 0
               : 1;
}

int assetDirectoryLayoutExists()
{
    const std::filesystem::path root = repoRoot();

    const bool hasAssets = requirePath(root / "assets");
    const bool hasTextures = requirePath(root / "assets" / "textures");
    const bool hasAudio = requirePath(root / "assets" / "audio");
    const bool hasModels = requirePath(root / "assets" / "models");
    const bool hasShaders = requirePath(root / "assets" / "shaders");
    const bool hasVideos = requirePath(root / "assets" / "videos");
    const bool hasFonts = requirePath(root / "assets" / "fonts");
    const bool hasScripts = requirePath(root / "assets" / "scripts");
    const bool hasTest = requirePath(root / "assets" / "test");
    const bool hasPrototypeScript = requirePath(root / "assets" / "scripts" / "test.vnscript");

    return hasAssets && hasTextures && hasAudio && hasModels && hasShaders && hasVideos &&
                   hasFonts && hasScripts && hasTest && hasPrototypeScript
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
    const bool hasMiniaudioHeader = requirePath(root / "external" / "miniaudio" / "miniaudio.h");

    return hasGlfw && hasGladHeader && hasGladSource && hasMiniaudioHeader ? 0 : 1;
}

int engineSourceLayoutExists()
{
    const std::filesystem::path root = repoRoot();

    const std::vector<std::filesystem::path> requiredFiles = {
        root / "src" / "main.cpp",
        root / "src" / "Application.h",
        root / "src" / "Application.cpp",
        root / "src" / "metassets" / "DaylightSandboxScene.metasset.h",
        root / "src" / "metassets" / "DaylightSandboxScene.metasset.cpp",
        root / "src" / "assets" / "Asset.h",
        root / "src" / "assets" / "Asset.cpp",
        root / "src" / "assets" / "AssetHandle.h",
        root / "src" / "assets" / "AssetManager.h",
        root / "src" / "assets" / "AssetManager.cpp",
        root / "src" / "assets" / "AssetMeta.h",
        root / "src" / "assets" / "AssetMeta.cpp",
        root / "src" / "assets" / "AssetRegistry.h",
        root / "src" / "assets" / "AssetRegistry.cpp",
        root / "src" / "assets" / "ShaderAsset.h",
        root / "src" / "assets" / "ShaderAsset.cpp",
        root / "src" / "assets" / "TextureAsset.h",
        root / "src" / "assets" / "TextureAsset.cpp",
        root / "src" / "assets" / "AudioAsset.h",
        root / "src" / "assets" / "AudioAsset.cpp",
        root / "src" / "core" / "AudioSystem.h",
        root / "src" / "core" / "AudioSystem.cpp",
        root / "src" / "assets" / "ModelAsset.h",
        root / "src" / "assets" / "ModelAsset.cpp",
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
        root / "src" / "runtime" / "VnScript.h",
        root / "src" / "runtime" / "VnScript.cpp",
        root / "src" / "runtime" / "VNRuntime.h",
        root / "src" / "runtime" / "VNRuntime.cpp",
        root / "src" / "runtime" / "RuntimeIds.h",
        root / "src" / "runtime" / "RuntimeIds.cpp",
        root / "src" / "runtime" / "RuntimeFactory.h",
        root / "src" / "runtime" / "RuntimeFactory.cpp",
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
        root / "src" / "scenes" / "AtmosphericSceneRuntime.h",
        root / "src" / "scenes" / "DaylightSandboxScene.h",
        root / "src" / "scenes" / "DaylightSandboxScene.cpp",
        root / "src" / "world" / "Camera.h",
        root / "src" / "world" / "Camera.cpp",
        root / "src" / "world" / "FreeCameraController.h",
        root / "src" / "world" / "FreeCameraController.cpp",
        root / "src" / "world" / "Lighting.h",
        root / "src" / "world" / "Material.h",
        root / "src" / "world" / "RayTracing.h",
        root / "src" / "world" / "Scene.h",
        root / "src" / "world" / "DaylightSandbox.h",
        root / "src" / "world" / "DaylightSandbox.cpp",
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

void writeTextFile(const std::filesystem::path& path, std::string_view content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << content;
}

void writeBinaryFile(const std::filesystem::path& path, const std::vector<unsigned char>& bytes)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
}

int assetManagerRoundTrip()
{
    namespace fs = std::filesystem;

    const fs::path tempRoot = fs::temp_directory_path() / "engine_asset_manager_smoke";
    std::error_code removeError;
    fs::remove_all(tempRoot, removeError);

    const fs::path audioPath = tempRoot / "assets" / "audio" / "wind.wav";
    const fs::path modelPath = tempRoot / "assets" / "models" / "spire.obj";
    const fs::path shaderRoot = tempRoot / "assets" / "shaders";
    const fs::path vertexShaderPath = shaderRoot / "test_surface.vert";
    const fs::path fragmentShaderPath = shaderRoot / "test_surface.frag";

    writeBinaryFile(audioPath, {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E'});
    writeTextFile(modelPath, "o Spire\nv 0 0 0\nv 0 1 0\nv 1 0 0\nf 1 2 3\n");
    writeTextFile(vertexShaderPath,
                  "#version 330 core\nlayout (location = 0) in vec3 aPosition;\nvoid main()\n{\n   "
                  " gl_Position = vec4(aPosition, 1.0);\n}\n");
    writeTextFile(
        fragmentShaderPath,
        "#version 330 core\nout vec4 FragColor;\nvoid main()\n{\n    FragColor = vec4(1.0);\n}\n");
    writeTextFile(audioPath.string() + ".meta", "uuid=wind-audio-smoke\n"
                                                "asset_type=audio\n"
                                                "tags=ambient, wind\n"
                                                "preload=true\n");

    engine::AssetManager manager;
    const std::size_t discoveredCount = manager.discover(tempRoot / "assets");
    if (discoveredCount != 4)
    {
        std::cerr << "Expected 4 discovered assets, found " << discoveredCount << '\n';
        return 1;
    }

    const auto audioHandle = manager.findByPath<engine::AudioAsset>(audioPath);
    if (!audioHandle || audioHandle.uuid() != "wind-audio-smoke")
    {
        std::cerr << "Audio asset lookup by path failed.\n";
        return 1;
    }

    const fs::path relativeAudioPath = fs::path("audio") / "wind.wav";
    const auto relativeAudioHandle = manager.findByPath<engine::AudioAsset>(relativeAudioPath);
    if (!relativeAudioHandle || relativeAudioHandle != audioHandle)
    {
        std::cerr << "Audio asset lookup by relative path failed.\n";
        return 1;
    }

    if (manager.resolveAssetPath(relativeAudioPath) != audioPath.lexically_normal())
    {
        std::cerr << "AssetManager did not resolve the relative audio path correctly.\n";
        return 1;
    }

    if (!manager.isLoaded(audioHandle.uuid()))
    {
        std::cerr << "Preload flag did not keep the audio asset resident.\n";
        return 1;
    }

    const std::shared_ptr<engine::AudioAsset> audio = manager.load(audioHandle);
    if (audio == nullptr || audio->sizeInBytes() == 0 || audio->formatName() != "wav")
    {
        std::cerr << "Audio placeholder asset failed to load.\n";
        return 1;
    }

    const auto audioLookupByUuid = manager.findByUuid<engine::AudioAsset>(audioHandle.uuid());
    if (audioLookupByUuid != audioHandle)
    {
        std::cerr << "Audio asset lookup by UUID did not round-trip.\n";
        return 1;
    }

    const auto modelHandle = manager.findByPath<engine::ModelAsset>(modelPath);
    if (!modelHandle || manager.isLoaded(modelHandle.uuid()))
    {
        std::cerr << "Model asset should be discoverable but lazily unloaded.\n";
        return 1;
    }

    const std::shared_ptr<engine::ModelAsset> model = manager.load(modelHandle);
    if (model == nullptr || model->sizeInBytes() == 0 || model->formatName() != "obj")
    {
        std::cerr << "Model placeholder asset failed to load.\n";
        return 1;
    }

    const auto modelLookupByUuid = manager.findByUuid<engine::ModelAsset>(modelHandle.uuid());
    if (modelLookupByUuid != modelHandle)
    {
        std::cerr << "Model asset lookup by UUID did not round-trip.\n";
        return 1;
    }

    const fs::path generatedModelMetaPath = modelPath.string() + ".meta";
    const fs::path generatedVertexMetaPath = vertexShaderPath.string() + ".meta";
    const fs::path generatedFragmentMetaPath = fragmentShaderPath.string() + ".meta";
    if (!fs::exists(generatedModelMetaPath) || !fs::exists(generatedVertexMetaPath) ||
        !fs::exists(generatedFragmentMetaPath))
    {
        std::cerr << "Missing generated metadata sidecars for discovered assets.\n";
        return 1;
    }

    const auto vertexHandle = manager.findByPath<engine::ShaderAsset>(vertexShaderPath);
    const auto fragmentHandle = manager.findByPath<engine::ShaderAsset>(fragmentShaderPath);
    if (!vertexHandle || !fragmentHandle)
    {
        std::cerr << "Shader assets should be discoverable through the asset manager.\n";
        return 1;
    }

    const auto relativeVertexHandle =
        manager.findByPath<engine::ShaderAsset>(fs::path("shaders") / "test_surface.vert");
    if (!relativeVertexHandle || relativeVertexHandle != vertexHandle)
    {
        std::cerr << "Shader asset lookup by relative path failed.\n";
        return 1;
    }

    const std::shared_ptr<engine::ShaderAsset> vertexShader = manager.load(vertexHandle);
    if (vertexShader == nullptr || vertexShader->stage() != engine::ShaderStage::Vertex ||
        vertexShader->source().empty())
    {
        std::cerr << "Vertex shader asset failed to load through the asset manager.\n";
        return 1;
    }

    const std::shared_ptr<engine::ShaderAsset> fragmentShader = manager.load(fragmentHandle);
    if (fragmentShader == nullptr || fragmentShader->stage() != engine::ShaderStage::Fragment ||
        fragmentShader->source().empty())
    {
        std::cerr << "Fragment shader asset failed to load through the asset manager.\n";
        return 1;
    }

    auto sharedManager = std::make_shared<engine::AssetManager>();
    sharedManager->discover(tempRoot / "assets");
    engine::ShaderLibrary shaderLibrary(sharedManager, shaderRoot);
    if (shaderLibrary.shaderPath("test_surface.vert") != vertexShaderPath ||
        shaderLibrary.shaderPath("test_surface.frag") != fragmentShaderPath)
    {
        std::cerr << "ShaderLibrary did not resolve shader assets through the asset manager.\n";
        return 1;
    }

    manager.unload(modelHandle.uuid());
    if (manager.isLoaded(modelHandle.uuid()))
    {
        std::cerr << "Model asset should be unloadable.\n";
        return 1;
    }

    fs::remove_all(tempRoot, removeError);
    return 0;
}

int vnscriptParserSmoke()
{
    const std::filesystem::path scriptPath = repoRoot() / "assets" / "scripts" / "test.vnscript";
    if (!std::filesystem::exists(scriptPath))
    {
        std::cerr << "Missing vnscript test asset: " << scriptPath.string() << '\n';
        return 1;
    }

    const engine::VnScript script = engine::parseVnScriptFile(scriptPath);
    if (script.instructions.size() < 22)
    {
        std::cerr << "Expected at least 22 vnscript instructions, found "
                  << script.instructions.size() << '\n';
        return 1;
    }

    if (script.instructions.front().type != engine::VnCommandType::CharacterSet ||
        script.instructions.front().identifier != "patrick")
    {
        std::cerr << "First vnscript command should define Patrick.\n";
        return 1;
    }

    if (script.instructions[1].type != engine::VnCommandType::Background ||
        script.instructions[1].assetPath != std::filesystem::path{"background.png"})
    {
        std::cerr << "Second vnscript command should set the placeholder background.\n";
        return 1;
    }

    int characterCommandCount = 0;
    bool foundCenterNativeScale = false;
    bool foundLeftScaledDown = false;
    bool foundRightScaledUp = false;
    bool foundHideCharacter = false;

    for (const engine::VnInstruction& instruction : script.instructions)
    {
        if (instruction.type == engine::VnCommandType::Character)
        {
            ++characterCommandCount;
            foundCenterNativeScale = foundCenterNativeScale ||
                                     (instruction.stageRegion == engine::VnStageRegion::Center &&
                                      instruction.scale == 1.0f);
            foundLeftScaledDown =
                foundLeftScaledDown || (instruction.stageRegion == engine::VnStageRegion::Left &&
                                        instruction.scale == 0.75f);
            foundRightScaledUp =
                foundRightScaledUp || (instruction.stageRegion == engine::VnStageRegion::Right &&
                                       instruction.scale == 1.5f);
        }

        if (instruction.type == engine::VnCommandType::HideCharacter &&
            instruction.identifier == "patrick")
        {
            foundHideCharacter = true;
        }
    }

    if (characterCommandCount < 3)
    {
        std::cerr << "Expected at least three CHARACTER commands in the prototype script.\n";
        return 1;
    }

    if (!foundCenterNativeScale)
    {
        std::cerr << "Expected the prototype script to include center native-scale staging.\n";
        return 1;
    }

    if (!foundLeftScaledDown)
    {
        std::cerr << "Expected the prototype script to include a smaller left-stage variant.\n";
        return 1;
    }

    if (!foundRightScaledUp)
    {
        std::cerr << "Expected the prototype script to include a larger right-stage variant.\n";
        return 1;
    }

    if (!foundHideCharacter)
    {
        std::cerr << "Expected the prototype script to hide Patrick before ending.\n";
        return 1;
    }

    if (script.instructions.back().type != engine::VnCommandType::End)
    {
        std::cerr << "Prototype vnscript must terminate with END.\n";
        return 1;
    }

    return 0;
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
        {"asset_directory_layout_exists", &assetDirectoryLayoutExists},
        {"dependency_layout_exists", &dependencyLayoutExists},
        {"engine_source_layout_exists", &engineSourceLayoutExists},
        {"asset_manager_round_trip", &assetManagerRoundTrip},
        {"vnscript_parser_smoke", &vnscriptParserSmoke}};

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
