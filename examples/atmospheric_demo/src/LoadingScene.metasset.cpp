#include "LoadingScene.metasset.h"

namespace engine
{
LoadingSceneMetasset::LoadingSceneMetasset() : SceneMetasset("loading")
{
    m_assetDependencies.push_back(SceneAssetDependency{
        kOverlayTextureId,
        AssetType::Texture,
        std::filesystem::path("textures") / "image.png",
        false,
    });
}

const SceneMetasset::ShaderProgramDependencies&
LoadingSceneMetasset::shaderProgramDependencies() const
{
    return m_shaderProgramDependencies;
}

const SceneMetasset::AssetDependencies& LoadingSceneMetasset::assetDependencies() const
{
    return m_assetDependencies;
}
} // namespace engine
