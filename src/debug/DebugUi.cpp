#include "debug/DebugUi.h"

#include "core/RenderProfiler.h"
#include "core/Renderer.h"
#include "world/Player.h"
#include "world/PlayerController.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdint>

namespace
{
enum class VolumetricTuningPreset
{
    RealisticNight = 0,
    CinematicHorror = 1,
    HeavyMist = 2,
    MoonlitValley = 3,
    VolumetricShowcase = 4,
};

const char* postDebugModeLabel(int mode)
{
    switch (static_cast<engine::PostDebugViewMode>(mode))
    {
    case engine::PostDebugViewMode::FinalImage:
        return "Final Image";
    case engine::PostDebugViewMode::HdrScene:
        return "HDR Scene";
    case engine::PostDebugViewMode::HdrLuminance:
        return "HDR Luminance";
    case engine::PostDebugViewMode::BloomExtract:
        return "Bloom Extract";
    case engine::PostDebugViewMode::ExposureApplied:
        return "Exposure Applied";
    case engine::PostDebugViewMode::ToneMapped:
        return "Tone Mapped";
    }

    return "Unknown";
}

const char* volumetricDebugModeLabel(int mode)
{
    switch (static_cast<engine::VolumetricDebugViewMode>(mode))
    {
    case engine::VolumetricDebugViewMode::FinalImage:
        return "Final Image";
    case engine::VolumetricDebugViewMode::FogDensity:
        return "Fog Density";
    case engine::VolumetricDebugViewMode::Transmittance:
        return "Transmittance";
    case engine::VolumetricDebugViewMode::Scattering:
        return "Scattering Accumulation";
    case engine::VolumetricDebugViewMode::RaymarchSteps:
        return "March Steps";
    case engine::VolumetricDebugViewMode::ShadowedFog:
        return "Shadowed Fog";
    case engine::VolumetricDebugViewMode::UnshadowedFog:
        return "Unshadowed Fog";
    case engine::VolumetricDebugViewMode::TemporalHistory:
        return "Temporal History";
    case engine::VolumetricDebugViewMode::VolumetricBufferResolution:
        return "Volumetric Buffer Resolution";
    case engine::VolumetricDebugViewMode::Extinction:
        return "Extinction";
    case engine::VolumetricDebugViewMode::LightEnergy:
        return "Light Energy";
    case engine::VolumetricDebugViewMode::PhaseFunction:
        return "Phase Function";
    case engine::VolumetricDebugViewMode::ShadowContribution:
        return "Shadow Contribution";
    case engine::VolumetricDebugViewMode::AtmosphericOcclusion:
        return "Atmospheric Occlusion";
    case engine::VolumetricDebugViewMode::TemporalHistoryWeight:
        return "Temporal History Weight";
    case engine::VolumetricDebugViewMode::RejectedReprojectionPixels:
        return "Rejected Reprojection Pixels";
    case engine::VolumetricDebugViewMode::ReprojectionVelocity:
        return "Reprojection Velocity";
    case engine::VolumetricDebugViewMode::ReprojectionUv:
        return "Reprojection UVs";
    case engine::VolumetricDebugViewMode::BilateralUpscaleMask:
        return "Bilateral Upscale Mask";
    case engine::VolumetricDebugViewMode::TemporalAccumulationConfidence:
        return "Temporal Accumulation Confidence";
    }

    return "Unknown";
}

const char* volumetricPresetLabel(VolumetricTuningPreset preset)
{
    switch (preset)
    {
    case VolumetricTuningPreset::RealisticNight:
        return "Realistic Night";
    case VolumetricTuningPreset::CinematicHorror:
        return "Cinematic Horror";
    case VolumetricTuningPreset::HeavyMist:
        return "Heavy Mist";
    case VolumetricTuningPreset::MoonlitValley:
        return "Moonlit Valley";
    case VolumetricTuningPreset::VolumetricShowcase:
        return "Volumetric Showcase";
    }

    return "Unknown";
}

void applyVolumetricPreset(engine::Scene& scene, VolumetricTuningPreset preset)
{
    scene.debugView.volumetricDebugViewMode =
        static_cast<int>(engine::VolumetricDebugViewMode::FinalImage);

    switch (preset)
    {
    case VolumetricTuningPreset::RealisticNight:
        scene.fog.density = 0.015f;
        scene.fog.heightFalloff = 0.80f;
        scene.rayEvaluation.scatteringStrength = 1.20f;
        scene.rayEvaluation.extinctionStrength = 1.50f;
        scene.rayEvaluation.phaseAnisotropy = 0.35f;
        scene.rayEvaluation.volumetricLightIntensity = 2.0f;
        scene.rayEvaluation.directionalLightAngularRadius = 0.004f;
        scene.rayEvaluation.stepLength = 1.0f;
        scene.rayEvaluation.maxSteps = 64;
        scene.rayEvaluation.atmosphericAmbientFloor = 0.015f;
        scene.rayEvaluation.maxDistance = 760.0f;
        scene.rayEvaluation.temporalBlend = 0.84f;
        scene.rayEvaluation.bilateralDepthFactor = 42.0f;
        scene.rayEvaluation.temporalDepthThreshold = 0.16f;
        scene.rayEvaluation.temporalNormalThreshold = 0.84f;
        scene.rayEvaluation.temporalVelocityThreshold = 4.0f;
        scene.rayEvaluation.nearFieldHaze = 0.0f;
        scene.rayEvaluation.jitterStrength = 0.14f;
        scene.rayEvaluation.stepDistributionExponent = 1.12f;
        scene.postProcess.exposure = 1.0f;
        scene.postProcess.bloomThreshold = 1.2f;
        scene.postProcess.bloomIntensity = 0.12f;
        break;
    case VolumetricTuningPreset::CinematicHorror:
        scene.fog.density = 0.026f;
        scene.fog.heightFalloff = 1.15f;
        scene.rayEvaluation.scatteringStrength = 2.30f;
        scene.rayEvaluation.extinctionStrength = 1.80f;
        scene.rayEvaluation.phaseAnisotropy = 0.42f;
        scene.rayEvaluation.volumetricLightIntensity = 3.60f;
        scene.rayEvaluation.directionalLightAngularRadius = 0.004f;
        scene.rayEvaluation.stepLength = 10.0f;
        scene.rayEvaluation.maxSteps = 256;
        scene.rayEvaluation.atmosphericAmbientFloor = 0.050f;
        scene.rayEvaluation.maxDistance = 1500.0f;
        scene.rayEvaluation.temporalBlend = 0.88f;
        scene.rayEvaluation.bilateralDepthFactor = 48.0f;
        scene.rayEvaluation.temporalDepthThreshold = 0.22f;
        scene.rayEvaluation.temporalNormalThreshold = 0.80f;
        scene.rayEvaluation.temporalVelocityThreshold = 5.0f;
        scene.rayEvaluation.nearFieldHaze = 0.25f;
        scene.rayEvaluation.jitterStrength = 0.40f;
        scene.rayEvaluation.stepDistributionExponent = 3.0f;
        scene.postProcess.exposure = 1.031f;
        scene.postProcess.bloomThreshold = 0.90f;
        scene.postProcess.bloomIntensity = 0.663f;
        break;
    case VolumetricTuningPreset::HeavyMist:
        scene.fog.density = 0.038f;
        scene.fog.heightFalloff = 0.45f;
        scene.rayEvaluation.scatteringStrength = 1.00f;
        scene.rayEvaluation.extinctionStrength = 1.15f;
        scene.rayEvaluation.phaseAnisotropy = 0.08f;
        scene.rayEvaluation.volumetricLightIntensity = 1.25f;
        scene.rayEvaluation.directionalLightAngularRadius = 0.004f;
        scene.rayEvaluation.stepLength = 1.20f;
        scene.rayEvaluation.maxSteps = 72;
        scene.rayEvaluation.atmosphericAmbientFloor = 0.060f;
        scene.rayEvaluation.maxDistance = 680.0f;
        scene.rayEvaluation.temporalBlend = 0.82f;
        scene.rayEvaluation.bilateralDepthFactor = 44.0f;
        scene.rayEvaluation.temporalDepthThreshold = 0.14f;
        scene.rayEvaluation.temporalNormalThreshold = 0.86f;
        scene.rayEvaluation.temporalVelocityThreshold = 3.5f;
        scene.rayEvaluation.nearFieldHaze = 0.18f;
        scene.rayEvaluation.jitterStrength = 0.10f;
        scene.rayEvaluation.stepDistributionExponent = 1.04f;
        scene.postProcess.exposure = 0.95f;
        scene.postProcess.bloomThreshold = 1.50f;
        scene.postProcess.bloomIntensity = 0.08f;
        break;
    case VolumetricTuningPreset::MoonlitValley:
        scene.fog.density = 0.022f;
        scene.fog.heightFalloff = 0.70f;
        scene.rayEvaluation.scatteringStrength = 1.35f;
        scene.rayEvaluation.extinctionStrength = 1.55f;
        scene.rayEvaluation.phaseAnisotropy = 0.45f;
        scene.rayEvaluation.volumetricLightIntensity = 2.40f;
        scene.rayEvaluation.directionalLightAngularRadius = 0.004f;
        scene.rayEvaluation.stepLength = 0.95f;
        scene.rayEvaluation.maxSteps = 80;
        scene.rayEvaluation.atmosphericAmbientFloor = 0.020f;
        scene.rayEvaluation.maxDistance = 820.0f;
        scene.rayEvaluation.temporalBlend = 0.85f;
        scene.rayEvaluation.bilateralDepthFactor = 46.0f;
        scene.rayEvaluation.temporalDepthThreshold = 0.18f;
        scene.rayEvaluation.temporalNormalThreshold = 0.84f;
        scene.rayEvaluation.temporalVelocityThreshold = 4.2f;
        scene.rayEvaluation.nearFieldHaze = 0.08f;
        scene.rayEvaluation.jitterStrength = 0.14f;
        scene.rayEvaluation.stepDistributionExponent = 1.16f;
        scene.postProcess.exposure = 0.95f;
        scene.postProcess.bloomThreshold = 1.10f;
        scene.postProcess.bloomIntensity = 0.12f;
        break;
    case VolumetricTuningPreset::VolumetricShowcase:
        scene.fog.density = 0.065f;
        scene.fog.heightFalloff = 1.30f;
        scene.rayEvaluation.scatteringStrength = 3.50f;
        scene.rayEvaluation.extinctionStrength = 1.10f;
        scene.rayEvaluation.phaseAnisotropy = 0.85f;
        scene.rayEvaluation.volumetricLightIntensity = 7.0f;
        scene.rayEvaluation.directionalLightAngularRadius = 0.004f;
        scene.rayEvaluation.stepLength = 0.70f;
        scene.rayEvaluation.maxSteps = 128;
        scene.rayEvaluation.atmosphericAmbientFloor = 0.035f;
        scene.rayEvaluation.maxDistance = 900.0f;
        scene.rayEvaluation.temporalBlend = 0.90f;
        scene.rayEvaluation.bilateralDepthFactor = 52.0f;
        scene.rayEvaluation.temporalDepthThreshold = 0.20f;
        scene.rayEvaluation.temporalNormalThreshold = 0.78f;
        scene.rayEvaluation.temporalVelocityThreshold = 5.5f;
        scene.rayEvaluation.nearFieldHaze = 0.12f;
        scene.rayEvaluation.jitterStrength = 0.18f;
        scene.rayEvaluation.stepDistributionExponent = 1.28f;
        scene.postProcess.exposure = 0.68f;
        scene.postProcess.bloomThreshold = 0.85f;
        scene.postProcess.bloomIntensity = 0.26f;
        break;
    }

    scene.rayEvaluation.phaseAnisotropy =
        std::clamp(scene.rayEvaluation.phaseAnisotropy, -0.95f, 0.95f);
}

const char* materialDebugModeLabel(int mode)
{
    switch (static_cast<engine::MaterialDebugViewMode>(mode))
    {
    case engine::MaterialDebugViewMode::Shaded:
        return "Shaded";
    case engine::MaterialDebugViewMode::MaterialIds:
        return "Material IDs";
    case engine::MaterialDebugViewMode::Roughness:
        return "Roughness";
    case engine::MaterialDebugViewMode::Specular:
        return "Specular";
    case engine::MaterialDebugViewMode::EmissiveOnly:
        return "Emissive Only";
    case engine::MaterialDebugViewMode::AtmosphereResponse:
        return "Atmosphere Response";
    case engine::MaterialDebugViewMode::LuminanceHeatmap:
        return "Luminance Heatmap";
    case engine::MaterialDebugViewMode::Normals:
        return "Normals";
    case engine::MaterialDebugViewMode::LightAttenuation:
        return "Light Attenuation";
    case engine::MaterialDebugViewMode::LightVolumes:
        return "Light Volumes";
    case engine::MaterialDebugViewMode::ShadowFactor:
        return "Shadow Factor";
    }

    return "Unknown";
}

ImTextureID toImGuiTexture(unsigned int textureId)
{
    return static_cast<ImTextureID>(static_cast<uintptr_t>(textureId));
}

const engine::PassPerformanceStats* findPassPerformance(const engine::FramePerformanceStats& stats,
                                                        const char* name)
{
    for (const engine::PassPerformanceStats& pass : stats.passes)
    {
        if (pass.name == name)
        {
            return &pass;
        }
    }

    return nullptr;
}
} // namespace

namespace engine
{
DebugUi::DebugUi(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

DebugUi::~DebugUi()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DebugUi::beginFrame() const
{
    if (!m_enabled)
    {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DebugUi::draw(Scene& scene, Player& player, PlayerController& playerController,
                   bool& debugFreeCameraEnabled, const FramePerformanceStats& performanceStats,
                   const RendererDebugTextures& debugTextures)
{
    if (!m_enabled)
    {
        return;
    }

    bool regenerateWorld = false;

    ImGui::Begin("Lighting / Moon");

    ImGui::TextUnformatted("Light Sources");
    ImGui::Checkbox("Moon Light", &scene.moonLightEnabled);
    ImGui::Checkbox("Sphere Lights", &scene.sphereLightsEnabled);
    ImGui::Checkbox("Cone Lights", &scene.coneLightsEnabled);
    ImGui::Separator();

    ImGui::TextUnformatted("Emissive Visibility");
    ImGui::Checkbox("Moon Emissive", &scene.moonEmissiveEnabled);
    ImGui::Checkbox("Sphere Emissive", &scene.sphereEmissiveEnabled);
    ImGui::Checkbox("Cone Emissive", &scene.coneEmissiveEnabled);
    ImGui::Separator();

    ImGui::TextUnformatted("Moon Motion");
    ImGui::Checkbox("Animate Moon", &scene.moonMotionEnabled);
    ImGui::SliderFloat("Moon Time Offset", &scene.moonTimeOffset, -120.0f, 120.0f);
    ImGui::Separator();

    ImGui::TextUnformatted("Moon Alignment");
    ImGui::Text("Light Direction: %.4f %.4f %.4f", scene.sunLight.direction.x,
                scene.sunLight.direction.y, scene.sunLight.direction.z);
    ImGui::Text("Current Moon Pos: %.3f %.3f %.3f", scene.moonVisualPosition.x,
                scene.moonVisualPosition.y, scene.moonVisualPosition.z);
    if (ImGui::Checkbox("Override Moon Visual Position", &scene.debugMoonVisualOverrideEnabled) &&
        scene.debugMoonVisualOverrideEnabled)
    {
        scene.debugMoonVisualOverridePosition = scene.moonVisualPosition;
    }
    if (ImGui::Button("Copy Current To Override"))
    {
        scene.debugMoonVisualOverridePosition = scene.moonVisualPosition;
    }
    if (scene.debugMoonVisualOverrideEnabled)
    {
        ImGui::InputFloat3("Moon Override Position", &scene.debugMoonVisualOverridePosition.x,
                           "%.3f");
        ImGui::DragFloat3("Moon Override Nudge", &scene.debugMoonVisualOverridePosition.x, 0.1f,
                          -2000.0f, 2000.0f, "%.3f");
        if (ImGui::Button("Use Derived Moon Position"))
        {
            scene.debugMoonVisualOverrideEnabled = false;
            scene.debugMoonVisualOverridePosition = scene.moonVisualPosition;
        }
    }
    else
    {
        ImGui::TextUnformatted("Moon visual is currently camera-relative to the light direction.");
    }
    ImGui::End();

    ImGui::Begin("Exploration");

    ImGui::TextUnformatted("Exploration");
    ImGui::Separator();

    ImGui::Checkbox("Debug Freecam", &debugFreeCameraEnabled);
    ImGui::Text("Player Mode: %s", debugFreeCameraEnabled ? "Debug Freecam" : "Grounded FPS");
    ImGui::Text("Grounded: %s", player.grounded() ? "yes" : "no");
    ImGui::Text("Player Position: %.1f %.1f %.1f", player.position().x, player.position().y,
                player.position().z);
    ImGui::Text("Player Velocity: %.2f %.2f %.2f", player.velocity().x, player.velocity().y,
                player.velocity().z);
    float walkSpeed = playerController.walkSpeed();
    if (ImGui::SliderFloat("Walk Speed", &walkSpeed, 2.0f, 12.0f))
    {
        playerController.setWalkSpeed(walkSpeed);
    }
    float sprintMultiplier = playerController.sprintMultiplier();
    if (ImGui::SliderFloat("Sprint Multiplier", &sprintMultiplier, 1.0f, 1.6f))
    {
        playerController.setSprintMultiplier(sprintMultiplier);
    }
    float jumpSpeed = playerController.jumpSpeed();
    if (ImGui::SliderFloat("Jump Speed", &jumpSpeed, 0.0f, 8.5f))
    {
        playerController.setJumpSpeed(jumpSpeed);
    }
    float gravity = playerController.gravity();
    if (ImGui::SliderFloat("Gravity", &gravity, 6.0f, 24.0f))
    {
        playerController.setGravity(gravity);
    }
    float mouseSensitivity = playerController.mouseSensitivity();
    if (ImGui::SliderFloat("Look Sensitivity", &mouseSensitivity, 0.03f, 0.18f))
    {
        playerController.setMouseSensitivity(mouseSensitivity);
    }
    ImGui::Separator();

    if (ImGui::Button("Recapture Mouse"))
    {
        m_shouldResumeCamera = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("Quit Game"))
    {
        m_shouldQuit = true;
    }

    ImGui::End();

    ImGui::Begin("World Generation");

    ImGui::TextUnformatted("World Generation");
    regenerateWorld |=
        ImGui::SliderInt("Terrain Density", &scene.proceduralWorld.terrainDensity, 20, 96);
    regenerateWorld |=
        ImGui::SliderFloat("Terrain Scale", &scene.proceduralWorld.terrainScale, 24.0f, 96.0f);
    regenerateWorld |=
        ImGui::SliderFloat("Terrain Height", &scene.proceduralWorld.terrainHeight, 4.0f, 24.0f);
    regenerateWorld |= ImGui::SliderInt("Tree Count", &scene.proceduralWorld.treeCount, 0, 180);
    regenerateWorld |= ImGui::SliderInt("Generation Seed", &scene.proceduralWorld.seed, 1, 4096);
    if (ImGui::Button("Rebuild World"))
    {
        regenerateWorld = true;
    }

    ImGui::SameLine();
    ImGui::Text("Objects: %d", static_cast<int>(scene.objects().size()));
    ImGui::Text("ECS: %d entities / %d component types / %d components",
                static_cast<int>(scene.registry().entityCount()),
                static_cast<int>(scene.registry().componentTypeCount()),
                static_cast<int>(scene.registry().totalComponentCount()));
    scene.proceduralWorld.regenerationRequested =
        scene.proceduralWorld.regenerationRequested || regenerateWorld;
    ImGui::End();

    ImGui::Begin("Debug Visibility");

    ImGui::TextUnformatted("Debug Visibility Paths");
    ImGui::Checkbox("Ambient", &scene.debugView.ambientEnabled);
    ImGui::Checkbox("Fog", &scene.debugView.fogEnabled);
    ImGui::Checkbox("Sky Lighting", &scene.debugView.skyLightingEnabled);
    ImGui::Checkbox("Emissive Propagation", &scene.debugView.emissivePropagationEnabled);
    if (ImGui::BeginCombo("Material Debug View",
                          materialDebugModeLabel(scene.debugView.materialDebugViewMode)))
    {
        for (int mode = static_cast<int>(MaterialDebugViewMode::Shaded);
             mode <= static_cast<int>(MaterialDebugViewMode::ShadowFactor); ++mode)
        {
            const bool selected = scene.debugView.materialDebugViewMode == mode;
            if (ImGui::Selectable(materialDebugModeLabel(mode), selected))
            {
                scene.debugView.materialDebugViewMode = mode;
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
    ImGui::Checkbox("Tone Mapping", &scene.debugView.toneMappingEnabled);
    ImGui::Checkbox("Post Processing", &scene.debugView.postProcessingEnabled);
    if (ImGui::BeginCombo("Post Debug View", postDebugModeLabel(scene.debugView.postDebugViewMode)))
    {
        for (int mode = static_cast<int>(PostDebugViewMode::FinalImage);
             mode <= static_cast<int>(PostDebugViewMode::ToneMapped); ++mode)
        {
            const bool selected = scene.debugView.postDebugViewMode == mode;
            if (ImGui::Selectable(postDebugModeLabel(mode), selected))
            {
                scene.debugView.postDebugViewMode = mode;
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
    ImGui::End();

    ImGui::Begin("Lighting Debug");
    ImGui::TextUnformatted("Renderer Surfaces");
    if (debugTextures.shadowMapTextureId != 0)
    {
        ImGui::TextUnformatted("Shadow Map");
        ImGui::Image(toImGuiTexture(debugTextures.shadowMapTextureId), ImVec2(220.0f, 220.0f),
                     ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    }
    if (debugTextures.sceneDepthTextureId != 0)
    {
        ImGui::TextUnformatted("Depth Buffer");
        ImGui::Image(toImGuiTexture(debugTextures.sceneDepthTextureId), ImVec2(220.0f, 140.0f),
                     ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    }
    if (debugTextures.volumetricTextureId != 0)
    {
        ImGui::TextUnformatted("Volumetric Resolve");
        ImGui::Image(toImGuiTexture(debugTextures.volumetricTextureId), ImVec2(220.0f, 140.0f),
                     ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    }
    ImGui::TextUnformatted(
        "Material Debug View covers normals, attenuation, light volumes, and shadow factor.");
    ImGui::End();

    ImGui::Begin("Volumetric / Atmosphere Tuning");

    static int selectedPreset = static_cast<int>(VolumetricTuningPreset::CinematicHorror);

    ImGui::TextUnformatted("Volumetric / Atmosphere Tuning");
    if (ImGui::BeginCombo(
            "Atmosphere Preset",
            volumetricPresetLabel(static_cast<VolumetricTuningPreset>(selectedPreset))))
    {
        for (int preset = static_cast<int>(VolumetricTuningPreset::RealisticNight);
             preset <= static_cast<int>(VolumetricTuningPreset::VolumetricShowcase); ++preset)
        {
            const bool selected = selectedPreset == preset;
            if (ImGui::Selectable(
                    volumetricPresetLabel(static_cast<VolumetricTuningPreset>(preset)), selected))
            {
                selectedPreset = preset;
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    if (ImGui::Button("Apply Atmosphere Preset"))
    {
        applyVolumetricPreset(scene, static_cast<VolumetricTuningPreset>(selectedPreset));
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Participating Media");
    ImGui::SliderFloat("Atmospheric Density", &scene.fog.density, 0.0f, 0.2f, "%.4f");
    ImGui::SliderFloat("Height Falloff", &scene.fog.heightFalloff, 0.0f, 4.0f, "%.3f");
    ImGui::SliderFloat("Scattering Strength", &scene.rayEvaluation.scatteringStrength, 0.0f, 8.0f,
                       "%.3f");
    ImGui::SliderFloat("Extinction Strength", &scene.rayEvaluation.extinctionStrength, 0.0f, 8.0f,
                       "%.3f");
    ImGui::SliderFloat("Phase Anisotropy", &scene.rayEvaluation.phaseAnisotropy, -0.95f, 0.95f,
                       "%.3f");
    ImGui::SliderFloat("Atmosphere Ambient Floor", &scene.rayEvaluation.atmosphericAmbientFloor,
                       0.0f, 0.2f, "%.4f");
    ImGui::SliderFloat("Fog Base Height", &scene.fog.baseHeight, -16.0f, 32.0f, "%.2f");
    ImGui::SliderFloat("Fog Max Height", &scene.fog.maxHeight, 8.0f, 240.0f, "%.1f");
    ImGui::SliderFloat("Near Field Haze", &scene.rayEvaluation.nearFieldHaze, 0.0f, 1.0f, "%.3f");

    ImGui::Separator();
    ImGui::TextUnformatted("Lighting Response");
    ImGui::SliderFloat("Light Intensity Multiplier", &scene.rayEvaluation.volumetricLightIntensity,
                       0.0f, 16.0f, "%.3f");
    ImGui::SliderFloat("Directional Angular Radius",
                       &scene.rayEvaluation.directionalLightAngularRadius, 0.0f, 0.02f, "%.4f rad");
    ImGui::SliderFloat("Shadow Strength", &scene.rayEvaluation.shadowStrength, 0.0f, 1.0f, "%.3f");

    ImGui::Separator();
    ImGui::TextUnformatted("Raymarch Integration");
    ImGui::SliderInt("Raymarch Step Count", &scene.rayEvaluation.maxSteps, 8, 256);
    ImGui::SliderFloat("Raymarch Step Length", &scene.rayEvaluation.stepLength, 0.1f, 10.0f,
                       "%.3f");
    ImGui::SliderFloat("Max March Distance", &scene.rayEvaluation.maxDistance, 16.0f, 1500.0f,
                       "%.1f");
    ImGui::SliderFloat("Step Distribution", &scene.rayEvaluation.stepDistributionExponent, 0.35f,
                       3.0f, "%.3f");
    ImGui::SliderFloat("Jitter Strength", &scene.rayEvaluation.jitterStrength, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat("Temporal Blend", &scene.rayEvaluation.temporalBlend, 0.0f, 0.98f, "%.3f");
    ImGui::SliderFloat("Depth Reject", &scene.rayEvaluation.temporalDepthThreshold, 0.05f, 4.0f,
                       "%.3f");
    ImGui::SliderFloat("Normal Reject", &scene.rayEvaluation.temporalNormalThreshold, 0.1f, 0.99f,
                       "%.3f");
    ImGui::SliderFloat("Velocity Reject", &scene.rayEvaluation.temporalVelocityThreshold, 1.0f,
                       32.0f, "%.2f px");
    ImGui::SliderFloat("Bilateral Depth", &scene.rayEvaluation.bilateralDepthFactor, 1.0f, 64.0f,
                       "%.2f");

    ImGui::Separator();
    ImGui::TextUnformatted("Display Response");
    ImGui::SliderFloat("Exposure", &scene.postProcess.exposure, 0.1f, 5.0f, "%.3f");
    ImGui::SliderFloat("Bloom Threshold", &scene.postProcess.bloomThreshold, 0.0f, 10.0f, "%.3f");
    ImGui::SliderFloat("Bloom Intensity", &scene.postProcess.bloomIntensity, 0.0f, 3.0f, "%.3f");
    ImGui::SliderFloat("Contrast", &scene.postProcess.contrast, 0.8f, 1.4f, "%.3f");
    ImGui::SliderFloat("Saturation", &scene.postProcess.saturation, 0.0f, 1.25f, "%.3f");
    ImGui::SliderFloat("Midtone Lift", &scene.postProcess.midtoneLift, 0.0f, 0.12f, "%.3f");
    ImGui::SliderFloat("Vignette", &scene.postProcess.vignetteStrength, 0.0f, 0.5f, "%.3f");

    ImGui::Separator();
    ImGui::TextUnformatted("Debug Visualization");
    if (ImGui::BeginCombo("Volumetric Debug View",
                          volumetricDebugModeLabel(scene.debugView.volumetricDebugViewMode)))
    {
        for (int mode = static_cast<int>(VolumetricDebugViewMode::FinalImage);
             mode <= static_cast<int>(VolumetricDebugViewMode::TemporalAccumulationConfidence);
             ++mode)
        {
            const bool selected = scene.debugView.volumetricDebugViewMode == mode;
            if (ImGui::Selectable(volumetricDebugModeLabel(mode), selected))
            {
                scene.debugView.volumetricDebugViewMode = mode;
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    scene.rayEvaluation.phaseAnisotropy =
        std::clamp(scene.rayEvaluation.phaseAnisotropy, -0.95f, 0.95f);

    ImGui::End();

    ImGui::Begin("Performance Debug");
    ImGui::Text("CPU Frame: %.2f ms", performanceStats.cpuFrameMilliseconds);
    ImGui::Text("GPU Frame: %.2f ms", performanceStats.gpuFrameMilliseconds);
    if (const PassPerformanceStats* volumetricPass =
            findPassPerformance(performanceStats, "Volumetric Fog Pass");
        volumetricPass != nullptr)
    {
        ImGui::Text("Volumetric Pass: CPU %.2f ms | GPU %.2f ms", volumetricPass->cpuMilliseconds,
                    volumetricPass->gpuMilliseconds);
    }
    if (const PassPerformanceStats* shadowPass =
            findPassPerformance(performanceStats, "Shadow Pass");
        shadowPass != nullptr)
    {
        ImGui::Text("Shadow Pass: CPU %.2f ms | GPU %.2f ms", shadowPass->cpuMilliseconds,
                    shadowPass->gpuMilliseconds);
    }
    ImGui::Text("Draw Calls: %d", performanceStats.drawCallCount);
    ImGui::Text("Volumetric Samples: %llu",
                static_cast<unsigned long long>(performanceStats.volumetricSampleCount));
    ImGui::Text("Raymarch Steps: %d", performanceStats.raymarchStepCount);
    ImGui::Text("Volumetric Buffer: %d x %d", performanceStats.volumetricBufferWidth,
                performanceStats.volumetricBufferHeight);
    ImGui::Separator();
    ImGui::TextUnformatted("Per-Pass Breakdown");
    for (const PassPerformanceStats& pass : performanceStats.passes)
    {
        ImGui::BulletText("%s | CPU %.2f ms | GPU %.2f ms", pass.name.c_str(), pass.cpuMilliseconds,
                          pass.gpuMilliseconds);
    }
    ImGui::End();
}

void DebugUi::endFrame() const
{
    if (!m_enabled)
    {
        return;
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool DebugUi::shouldQuit() const noexcept
{
    return m_shouldQuit;
}

bool DebugUi::consumeResumeCameraRequest() noexcept
{
    const bool shouldResumeCamera = m_shouldResumeCamera;
    m_shouldResumeCamera = false;
    return shouldResumeCamera;
}

void DebugUi::setEnabled(bool enabled) noexcept
{
    m_enabled = enabled;
    if (!enabled)
    {
        m_shouldQuit = false;
        m_shouldResumeCamera = false;
    }
}

bool DebugUi::isEnabled() const noexcept
{
    return m_enabled;
}
} // namespace engine
