#include "metassets/MenuScene.metasset.h"

namespace engine
{
MenuSceneMetasset::MenuSceneMetasset() : SceneMetasset("menu")
{
    m_assetDependencies.push_back(SceneAssetDependency{
        kOverlayTextureId,
        AssetType::Texture,
        std::filesystem::path("textures") / "image.png",
        false,
    });
}

const SceneMetasset::ShaderProgramDependencies& MenuSceneMetasset::shaderProgramDependencies() const
{
    return m_shaderProgramDependencies;
}

const SceneMetasset::AssetDependencies& MenuSceneMetasset::assetDependencies() const
{
    return m_assetDependencies;
}
} // namespace engine
