#pragma once

#include "metassets/SceneMetasset.h"

namespace engine
{
class DaylightSandboxSceneMetasset final : public SceneMetasset
{
  public:
    static constexpr const char* kSurfaceShaderId = "surface.forward";

    DaylightSandboxSceneMetasset();
    ~DaylightSandboxSceneMetasset() override = default;

    const ShaderProgramDependencies& shaderProgramDependencies() const override;
    const AssetDependencies& assetDependencies() const override;

  private:
    ShaderProgramDependencies m_shaderProgramDependencies;
    AssetDependencies m_assetDependencies;
};
} // namespace engine
