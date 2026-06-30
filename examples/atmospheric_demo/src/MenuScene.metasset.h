#pragma once

#include "SceneMetasset.h"

namespace engine
{
class MenuSceneMetasset final : public SceneMetasset
{
  public:
    static constexpr const char* kOverlayTextureId = "menu.overlay";

    MenuSceneMetasset();
    ~MenuSceneMetasset() override = default;

    const ShaderProgramDependencies& shaderProgramDependencies() const override;
    const AssetDependencies& assetDependencies() const override;

  private:
    ShaderProgramDependencies m_shaderProgramDependencies;
    AssetDependencies m_assetDependencies;
};
} // namespace engine
