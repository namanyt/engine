#pragma once

#include "math/Types.h"

namespace engine
{
enum class LocalLightGroup
{
    Moon,
    Sphere,
    Cone,
};

enum class PostDebugViewMode
{
    FinalImage = 0,
    HdrScene = 1,
    HdrLuminance = 2,
    BloomExtract = 3,
    ExposureApplied = 4,
    ToneMapped = 5,
};

enum class VolumetricDebugViewMode
{
    Composite = 0,
    RaySteps = 1,
    DensityAccumulation = 2,
    ScatteringOnly = 3,
    IntegrationHeatmap = 4,
    SampleCount = 5,
};

enum class MaterialDebugViewMode
{
    Shaded = 0,
    MaterialIds = 1,
    Roughness = 2,
    Specular = 3,
    EmissiveOnly = 4,
    AtmosphereResponse = 5,
    LuminanceHeatmap = 6,
};

struct FogSettings final
{
    Vec3 color{0.18f, 0.24f, 0.30f};
    float density = 0.02f;
    float baseHeight = 3.0f;
    float heightFalloff = 0.08f;
};

struct DirectionalLight final
{
    Vec3 direction{-0.45f, -1.0f, -0.25f};
    Vec3 color{0.78f, 0.84f, 0.96f};
    float intensity = 1.2f;
};

struct LocalLight final
{
    Vec3 position{};
    Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float baseIntensity = 1.0f;
    bool enabled = true;
    bool followMoon = false;
    LocalLightGroup group = LocalLightGroup::Sphere;
};

struct ShadowSettings final
{
    int mapSize = 2048;
    float projectionRadius = 72.0f;
    float nearPlane = 1.0f;
    float farPlane = 180.0f;
    float bias = 0.0018f;
    float normalBias = 0.025f;
    float strength = 0.72f;
    Vec3 focusPoint{0.0f, 8.0f, -44.0f};
};

struct SkyLight final
{
    Vec3 horizonColor{0.16f, 0.21f, 0.27f};
    Vec3 zenithColor{0.03f, 0.05f, 0.08f};
    Vec3 groundColor{0.03f, 0.03f, 0.04f};
    float intensity = 0.42f;
};

struct PostProcessSettings final
{
    float exposure = 1.0f;
    float bloomThreshold = 1.35f;
    float bloomIntensity = 0.10f;
    float gamma = 2.2f;
    float contrast = 1.02f;
    float vignetteStrength = 0.14f;
    float saturation = 1.0f;
    float midtoneLift = 0.0f;
};

struct RayEvaluationSettings final
{
    float shadowStrength = 0.18f;
    float atmosphereIntensity = 0.95f;
    float emissiveScatter = 1.35f;
    float stepLength = 0.85f;
    float maxDistance = 160.0f;
    int maxSteps = 96;
    float extinction = 0.58f;
    float nearFieldHaze = 0.62f;
    float phaseAnisotropy = 0.68f;
    float jitterStrength = 0.92f;
    float stepDistributionExponent = 1.55f;
    float temporalJitterScale = 0.0f;
};

struct DebugViewSettings final
{
    bool ambientEnabled = true;
    bool fogEnabled = true;
    bool postProcessingEnabled = true;
    bool toneMappingEnabled = true;
    bool skyLightingEnabled = true;
    bool emissivePropagationEnabled = true;
    int postDebugViewMode = static_cast<int>(PostDebugViewMode::FinalImage);
    int volumetricDebugViewMode = static_cast<int>(VolumetricDebugViewMode::Composite);
    int materialDebugViewMode = static_cast<int>(MaterialDebugViewMode::Shaded);
};
} // namespace engine
