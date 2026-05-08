#pragma once

#include "metassets/SceneMetasset.h"

namespace engine
{
class TestWorldSceneMetasset final : public SceneMetasset
{
  public:
    static constexpr const char* kSurfaceShaderId = "surface.forward";
    static constexpr const char* kOverlayTextureId = "runtime.overlay";

    TestWorldSceneMetasset();
    ~TestWorldSceneMetasset() override = default;

    const ShaderProgramDependencies& shaderProgramDependencies() const override;
    const AssetDependencies& assetDependencies() const override;

  private:
    ShaderProgramDependencies m_shaderProgramDependencies;
    AssetDependencies m_assetDependencies;
};
} // namespace engine