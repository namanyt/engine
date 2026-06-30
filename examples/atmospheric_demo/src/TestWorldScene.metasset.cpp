#include "TestWorldScene.metasset.h"

namespace engine
{
TestWorldSceneMetasset::TestWorldSceneMetasset() : SceneMetasset("test_world")
{
    m_shaderProgramDependencies.push_back(SceneShaderProgramDependency{
        kSurfaceShaderId,
        "scenes.test_world.surface.forward",
        "vertex.glsl",
        "fragment.glsl",
    });

    m_assetDependencies.push_back(SceneAssetDependency{
        kOverlayTextureId,
        AssetType::Texture,
        std::filesystem::path("textures") / "image.png",
        false,
    });
}

const SceneMetasset::ShaderProgramDependencies&
TestWorldSceneMetasset::shaderProgramDependencies() const
{
    return m_shaderProgramDependencies;
}

const SceneMetasset::AssetDependencies& TestWorldSceneMetasset::assetDependencies() const
{
    return m_assetDependencies;
}
} // namespace engine