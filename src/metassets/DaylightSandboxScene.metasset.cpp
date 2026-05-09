#include "metassets/DaylightSandboxScene.metasset.h"

namespace engine
{
DaylightSandboxSceneMetasset::DaylightSandboxSceneMetasset() : SceneMetasset("daylight_sandbox")
{
    m_shaderProgramDependencies.push_back(SceneShaderProgramDependency{
        kSurfaceShaderId,
        "scenes.daylight_sandbox.surface.forward",
        "vertex.glsl",
        "fragment.glsl",
    });
}

const SceneMetasset::ShaderProgramDependencies&
DaylightSandboxSceneMetasset::shaderProgramDependencies() const
{
    return m_shaderProgramDependencies;
}

const SceneMetasset::AssetDependencies& DaylightSandboxSceneMetasset::assetDependencies() const
{
    return m_assetDependencies;
}
} // namespace engine
