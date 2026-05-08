#pragma once

#include "assets/AssetMeta.h"
#include "metassets/Metasset.h"

#include <filesystem>
#include <string>
#include <vector>

namespace engine
{
struct SceneShaderProgramDependency final
{
    std::string id;
    std::string programKey;
    std::filesystem::path vertexShaderPath;
    std::filesystem::path fragmentShaderPath;
};

struct SceneAssetDependency final
{
    std::string id;
    AssetType type = AssetType::Unknown;
    std::filesystem::path assetPath;
    bool required = true;
};

class SceneMetasset : public Metasset
{
  public:
    using ShaderProgramDependencies = std::vector<SceneShaderProgramDependency>;
    using AssetDependencies = std::vector<SceneAssetDependency>;

    explicit SceneMetasset(std::string name);
    ~SceneMetasset() override;

    virtual const ShaderProgramDependencies& shaderProgramDependencies() const = 0;
    virtual const AssetDependencies& assetDependencies() const = 0;
};
} // namespace engine
