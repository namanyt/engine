#pragma once

#include "math/Types.h"

namespace engine
{
class Shader;

enum class MaterialCategory
{
    MatteStone = 1,
    WetReflective = 2,
    MetallicStructure = 3,
    EmissiveBeacon = 4,
    DenseAbsorptive = 5,
    FogReactive = 6,
};

struct Material final
{
    const Shader* shader = nullptr;
    MaterialCategory category = MaterialCategory::MatteStone;
    Vec3 albedo{1.0f, 1.0f, 1.0f};
    Vec3 emissiveColor{};
    float emissiveStrength = 0.0f;
    float baseEmissiveStrength = 0.0f;
    float roughness = 0.72f;
    float metallic = 0.0f;
    float specularStrength = 0.28f;
    float softness = 0.0f;
    float atmosphericResponse = 1.0f;
};
} // namespace engine
