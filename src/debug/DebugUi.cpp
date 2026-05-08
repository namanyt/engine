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

void applyVolumetricPreset(engine::AtmosphericRenderSettings& renderSettings,
                           VolumetricTuningPreset preset)
{
    renderSettings.debugView.volumetricDebugViewMode =
        static_cast<int>(engine::VolumetricDebugViewMode::FinalImage);

    switch (preset)
    {
    case VolumetricTuningPreset::RealisticNight:
        renderSettings.fog.density = 0.015f;
        renderSettings.fog.heightFalloff = 0.80f;
        renderSettings.rayEvaluation.scatteringStrength = 1.20f;
        renderSettings.rayEvaluation.extinctionStrength = 1.50f;
        renderSettings.rayEvaluation.phaseAnisotropy = 0.35f;
        renderSettings.rayEvaluation.volumetricLightIntensity = 2.0f;
        renderSettings.rayEvaluation.directionalLightAngularRadius = 0.004f;
        renderSettings.rayEvaluation.stepLength = 1.0f;
        renderSettings.rayEvaluation.maxSteps = 64;
        renderSettings.rayEvaluation.atmosphericAmbientFloor = 0.015f;
        renderSettings.rayEvaluation.maxDistance = 760.0f;
        renderSettings.rayEvaluation.temporalBlend = 0.84f;
        renderSettings.rayEvaluation.bilateralDepthFactor = 42.0f;
        renderSettings.rayEvaluation.temporalDepthThreshold = 0.16f;
        renderSettings.rayEvaluation.temporalNormalThreshold = 0.84f;
        renderSettings.rayEvaluation.temporalVelocityThreshold = 4.0f;
        renderSettings.rayEvaluation.nearFieldHaze = 0.0f;
        renderSettings.rayEvaluation.jitterStrength = 0.14f;
        renderSettings.rayEvaluation.stepDistributionExponent = 1.12f;
        renderSettings.postProcess.exposure = 1.0f;
        renderSettings.postProcess.bloomThreshold = 1.2f;
        renderSettings.postProcess.bloomIntensity = 0.12f;
        break;
    case VolumetricTuningPreset::CinematicHorror:
        renderSettings.fog.density = 0.026f;
        renderSettings.fog.heightFalloff = 1.15f;
        renderSettings.rayEvaluation.scatteringStrength = 2.30f;
        renderSettings.rayEvaluation.extinctionStrength = 1.80f;
        renderSettings.rayEvaluation.phaseAnisotropy = 0.42f;
        renderSettings.rayEvaluation.volumetricLightIntensity = 3.60f;
        renderSettings.rayEvaluation.directionalLightAngularRadius = 0.004f;
        renderSettings.rayEvaluation.stepLength = 10.0f;
        renderSettings.rayEvaluation.maxSteps = 256;
        renderSettings.rayEvaluation.atmosphericAmbientFloor = 0.050f;
        renderSettings.rayEvaluation.maxDistance = 1500.0f;
        renderSettings.rayEvaluation.temporalBlend = 0.88f;
        renderSettings.rayEvaluation.bilateralDepthFactor = 48.0f;
        renderSettings.rayEvaluation.temporalDepthThreshold = 0.22f;
        renderSettings.rayEvaluation.temporalNormalThreshold = 0.80f;
        renderSettings.rayEvaluation.temporalVelocityThreshold = 5.0f;
        renderSettings.rayEvaluation.nearFieldHaze = 0.25f;
        renderSettings.rayEvaluation.jitterStrength = 0.40f;
        renderSettings.rayEvaluation.stepDistributionExponent = 3.0f;
        renderSettings.postProcess.exposure = 1.031f;
        renderSettings.postProcess.bloomThreshold = 0.90f;
        renderSettings.postProcess.bloomIntensity = 0.663f;
        break;
    case VolumetricTuningPreset::HeavyMist:
        renderSettings.fog.density = 0.038f;
        renderSettings.fog.heightFalloff = 0.45f;
        renderSettings.rayEvaluation.scatteringStrength = 1.00f;
        renderSettings.rayEvaluation.extinctionStrength = 1.15f;
        renderSettings.rayEvaluation.phaseAnisotropy = 0.08f;
        renderSettings.rayEvaluation.volumetricLightIntensity = 1.25f;
        renderSettings.rayEvaluation.directionalLightAngularRadius = 0.004f;
        renderSettings.rayEvaluation.stepLength = 1.20f;
        renderSettings.rayEvaluation.maxSteps = 72;
        renderSettings.rayEvaluation.atmosphericAmbientFloor = 0.060f;
        renderSettings.rayEvaluation.maxDistance = 680.0f;
        renderSettings.rayEvaluation.temporalBlend = 0.82f;
        renderSettings.rayEvaluation.bilateralDepthFactor = 44.0f;
        renderSettings.rayEvaluation.temporalDepthThreshold = 0.14f;
        renderSettings.rayEvaluation.temporalNormalThreshold = 0.86f;
        renderSettings.rayEvaluation.temporalVelocityThreshold = 3.5f;
        renderSettings.rayEvaluation.nearFieldHaze = 0.18f;
        renderSettings.rayEvaluation.jitterStrength = 0.10f;
        renderSettings.rayEvaluation.stepDistributionExponent = 1.04f;
        renderSettings.postProcess.exposure = 0.95f;
        renderSettings.postProcess.bloomThreshold = 1.50f;
        renderSettings.postProcess.bloomIntensity = 0.08f;
        break;
    case VolumetricTuningPreset::MoonlitValley:
        renderSettings.fog.density = 0.022f;
        renderSettings.fog.heightFalloff = 0.70f;
        renderSettings.rayEvaluation.scatteringStrength = 1.35f;
        renderSettings.rayEvaluation.extinctionStrength = 1.55f;
        renderSettings.rayEvaluation.phaseAnisotropy = 0.45f;
        renderSettings.rayEvaluation.volumetricLightIntensity = 2.40f;
        renderSettings.rayEvaluation.directionalLightAngularRadius = 0.004f;
        renderSettings.rayEvaluation.stepLength = 0.95f;
        renderSettings.rayEvaluation.maxSteps = 80;
        renderSettings.rayEvaluation.atmosphericAmbientFloor = 0.020f;
        renderSettings.rayEvaluation.maxDistance = 820.0f;
        renderSettings.rayEvaluation.temporalBlend = 0.85f;
        renderSettings.rayEvaluation.bilateralDepthFactor = 46.0f;
        renderSettings.rayEvaluation.temporalDepthThreshold = 0.18f;
        renderSettings.rayEvaluation.temporalNormalThreshold = 0.84f;
        renderSettings.rayEvaluation.temporalVelocityThreshold = 4.2f;
        renderSettings.rayEvaluation.nearFieldHaze = 0.08f;
        renderSettings.rayEvaluation.jitterStrength = 0.14f;
        renderSettings.rayEvaluation.stepDistributionExponent = 1.16f;
        renderSettings.postProcess.exposure = 0.95f;
        renderSettings.postProcess.bloomThreshold = 1.10f;
        renderSettings.postProcess.bloomIntensity = 0.12f;
        break;
    case VolumetricTuningPreset::VolumetricShowcase:
        renderSettings.fog.density = 0.065f;
        renderSettings.fog.heightFalloff = 1.30f;
        renderSettings.rayEvaluation.scatteringStrength = 3.50f;
        renderSettings.rayEvaluation.extinctionStrength = 1.10f;
        renderSettings.rayEvaluation.phaseAnisotropy = 0.85f;
        renderSettings.rayEvaluation.volumetricLightIntensity = 7.0f;
        renderSettings.rayEvaluation.directionalLightAngularRadius = 0.004f;
        renderSettings.rayEvaluation.stepLength = 0.70f;
        renderSettings.rayEvaluation.maxSteps = 128;
        renderSettings.rayEvaluation.atmosphericAmbientFloor = 0.035f;
        renderSettings.rayEvaluation.maxDistance = 900.0f;
        renderSettings.rayEvaluation.temporalBlend = 0.90f;
        renderSettings.rayEvaluation.bilateralDepthFactor = 52.0f;
        renderSettings.rayEvaluation.temporalDepthThreshold = 0.20f;
        renderSettings.rayEvaluation.temporalNormalThreshold = 0.78f;
        renderSettings.rayEvaluation.temporalVelocityThreshold = 5.5f;
        renderSettings.rayEvaluation.nearFieldHaze = 0.12f;
        renderSettings.rayEvaluation.jitterStrength = 0.18f;
        renderSettings.rayEvaluation.stepDistributionExponent = 1.28f;
        renderSettings.postProcess.exposure = 0.68f;
        renderSettings.postProcess.bloomThreshold = 0.85f;
        renderSettings.postProcess.bloomIntensity = 0.26f;
        break;
    }

    renderSettings.rayEvaluation.phaseAnisotropy =
        std::clamp(renderSettings.rayEvaluation.phaseAnisotropy, -0.95f, 0.95f);
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

void DebugUi::draw(AtmosphericWorldSettings& worldSettings,
                   AtmosphericRenderSettings& renderSettings, AtmosphericRuntimeState& runtimeState,
                   const ExplorationRuntimeStats& stats, Player& player,
                   PlayerController& playerController, bool& debugFreeCameraEnabled,
                   const FramePerformanceStats& performanceStats,
                   const RendererDebugTextures& debugTextures)
{
    if (!m_enabled)
    {
        return;
    }

    bool regenerateWorld = false;

    ImGui::Begin("Lighting / Moon");

    ImGui::TextUnformatted("Light Sources");
    ImGui::Checkbox("Moon Light", &worldSettings.moonLightEnabled);
    ImGui::Checkbox("Sphere Lights", &worldSettings.sphereLightsEnabled);
    ImGui::Checkbox("Cone Lights", &worldSettings.coneLightsEnabled);
    ImGui::Separator();

    ImGui::TextUnformatted("Emissive Visibility");
    ImGui::Checkbox("Moon Emissive", &worldSettings.moonEmissiveEnabled);
    ImGui::Checkbox("Sphere Emissive", &worldSettings.sphereEmissiveEnabled);
    ImGui::Checkbox("Cone Emissive", &worldSettings.coneEmissiveEnabled);
    ImGui::Separator();

    ImGui::TextUnformatted("Moon Motion");
    ImGui::Checkbox("Animate Moon", &worldSettings.moonMotionEnabled);
    ImGui::SliderFloat("Moon Time Offset", &worldSettings.moonTimeOffset, -120.0f, 120.0f);
    ImGui::Separator();

    ImGui::TextUnformatted("Moon Alignment");
    ImGui::Text("Light Direction: %.4f %.4f %.4f", renderSettings.sunLight.direction.x,
                renderSettings.sunLight.direction.y, renderSettings.sunLight.direction.z);
    ImGui::Text("Current Moon Pos: %.3f %.3f %.3f", runtimeState.moonVisualPosition.x,
                runtimeState.moonVisualPosition.y, runtimeState.moonVisualPosition.z);
    if (ImGui::Checkbox("Override Moon Visual Position",
                        &runtimeState.debugMoonVisualOverrideEnabled) &&
        runtimeState.debugMoonVisualOverrideEnabled)
    {
        runtimeState.debugMoonVisualOverridePosition = runtimeState.moonVisualPosition;
    }
    if (ImGui::Button("Copy Current To Override"))
    {
        runtimeState.debugMoonVisualOverridePosition = runtimeState.moonVisualPosition;
    }
    if (runtimeState.debugMoonVisualOverrideEnabled)
    {
        ImGui::InputFloat3("Moon Override Position",
                           &runtimeState.debugMoonVisualOverridePosition.x, "%.3f");
        ImGui::DragFloat3("Moon Override Nudge", &runtimeState.debugMoonVisualOverridePosition.x,
                          0.1f, -2000.0f, 2000.0f, "%.3f");
        if (ImGui::Button("Use Derived Moon Position"))
        {
            runtimeState.debugMoonVisualOverrideEnabled = false;
            runtimeState.debugMoonVisualOverridePosition = runtimeState.moonVisualPosition;
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
    ImGui::TextUnformatted("F1 toggles menu visibility. Esc toggles cursor capture.");
    ImGui::Text("Cursor Captured: %s", runtimeState.movementDebug.cursorCaptured ? "yes" : "no");

    ImGui::Checkbox("Debug Freecam", &debugFreeCameraEnabled);
    ImGui::Text("Player Mode: %s", debugFreeCameraEnabled ? "Debug Freecam" : "Grounded FPS");
    ImGui::Text("Grounded: %s", player.grounded() ? "yes" : "no");
    ImGui::Text("Player Position: %.1f %.1f %.1f", player.position().x, player.position().y,
                player.position().z);
    ImGui::Text("Player Velocity: %.2f %.2f %.2f", player.velocity().x, player.velocity().y,
                player.velocity().z);
    ImGui::Text("Frame Delta: %.3f ms", runtimeState.movementDebug.deltaSeconds * 1000.0f);
    ImGui::Text("Fixed Step / Steps / Accumulator: %.3f ms / %d / %.3f ms",
                runtimeState.movementDebug.simulationStepSeconds * 1000.0f,
                runtimeState.movementDebug.fixedSteps,
                runtimeState.movementDebug.accumulatorSeconds * 1000.0f);
    ImGui::Text("Dropped Sim / Present Alpha / Clamped: %.3f ms / %.2f / %s",
                runtimeState.movementDebug.droppedSimulationSeconds * 1000.0f,
                runtimeState.movementDebug.presentationAlpha,
                runtimeState.movementDebug.simulationClamped ? "yes" : "no");
    ImGui::Text("Terrain Height / Support Height: %.2f / %.2f",
                runtimeState.movementDebug.terrainHeight, runtimeState.movementDebug.supportHeight);
    ImGui::Text("Slope Angle / Support Distance: %.2f deg / %.3f",
                runtimeState.movementDebug.slopeAngleDegrees,
                runtimeState.movementDebug.supportDistance);
    ImGui::Text("Support Persist / Retained: %.3f / %s",
                runtimeState.movementDebug.supportPersistenceRemaining,
                runtimeState.movementDebug.supportRetained ? "yes" : "no");
    ImGui::Text("Input Direction: %.2f %.2f %.2f", runtimeState.movementDebug.inputDirection.x,
                runtimeState.movementDebug.inputDirection.y,
                runtimeState.movementDebug.inputDirection.z);
    ImGui::Text("Desired Velocity: %.2f %.2f %.2f", runtimeState.movementDebug.desiredVelocity.x,
                runtimeState.movementDebug.desiredVelocity.y,
                runtimeState.movementDebug.desiredVelocity.z);
    ImGui::Text("Projected Velocity: %.2f %.2f %.2f",
                runtimeState.movementDebug.projectedVelocity.x,
                runtimeState.movementDebug.projectedVelocity.y,
                runtimeState.movementDebug.projectedVelocity.z);
    ImGui::Text("Collision Count / Sweep Iterations: %d / %d",
                runtimeState.movementDebug.collisionCount,
                runtimeState.movementDebug.sweepIterations);
    ImGui::Text("Collision Triangles / Penetration Recoveries: %d / %d",
                runtimeState.movementDebug.collisionTriangleCount,
                runtimeState.movementDebug.penetrationRecoveries);
    ImGui::Text("Cache Rebuilt / Stale Collider Change: %s / %s",
                runtimeState.movementDebug.collisionCacheRebuilt ? "yes" : "no",
                runtimeState.movementDebug.staleColliderDetected ? "yes" : "no");
    ImGui::Text("Last Collision Normal: %.2f %.2f %.2f",
                runtimeState.movementDebug.lastCollisionNormal.x,
                runtimeState.movementDebug.lastCollisionNormal.y,
                runtimeState.movementDebug.lastCollisionNormal.z);
    ImGui::Text("Last Surface Motion: %.3f %.3f %.3f",
                runtimeState.movementDebug.lastSurfaceMotion.x,
                runtimeState.movementDebug.lastSurfaceMotion.y,
                runtimeState.movementDebug.lastSurfaceMotion.z);
    ImGui::Text("Support Normal: %.2f %.2f %.2f", runtimeState.movementDebug.supportNormal.x,
                runtimeState.movementDebug.supportNormal.y,
                runtimeState.movementDebug.supportNormal.z);
    ImGui::Text("Support Point: %.2f %.2f %.2f", runtimeState.movementDebug.supportPoint.x,
                runtimeState.movementDebug.supportPoint.y,
                runtimeState.movementDebug.supportPoint.z);
    ImGui::Text("Camera Offset / Landing Dip: %.3f %.3f %.3f / %.3f",
                runtimeState.movementDebug.cameraOffset.x,
                runtimeState.movementDebug.cameraOffset.y,
                runtimeState.movementDebug.cameraOffset.z, runtimeState.movementDebug.landingDip);
    ImGui::Text("Capsule Radius / Height: %.2f / %.2f", runtimeState.movementDebug.capsuleRadius,
                runtimeState.movementDebug.capsuleHeight);
    ImGui::Text("Traversal: crouch=%s step-up=%s support=%s",
                runtimeState.movementDebug.crouching ? "yes" : "no",
                runtimeState.movementDebug.stepUpApplied ? "yes" : "no",
                runtimeState.movementDebug.supportHit ? "yes" : "no");
    ImGui::Text("Grounded Duration / Coyote / Jump Buffer: %.3f / %.3f / %.3f",
                runtimeState.movementDebug.groundedDuration,
                runtimeState.movementDebug.coyoteTimeRemaining,
                runtimeState.movementDebug.jumpBufferRemaining);
    ImGui::Text("Support Acquisitions / Ground->Air / Air->Ground: %d / %d / %d",
                runtimeState.movementDebug.supportAcquisitionCount,
                runtimeState.movementDebug.airborneTransitionCount,
                runtimeState.movementDebug.groundedTransitionCount);
    ImGui::Text("Friction Impulse / Horizontal Momentum: %.3f / %.3f",
                runtimeState.movementDebug.frictionImpulse,
                runtimeState.movementDebug.horizontalMomentumRatio);
    ImGui::Text("Residual Motion / Sweep Failure: %.4f / %s",
                runtimeState.movementDebug.residualMotionLength,
                runtimeState.movementDebug.sweepFailureDetected ? "yes" : "no");
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
    float groundAcceleration = playerController.groundAcceleration();
    if (ImGui::SliderFloat("Ground Accel", &groundAcceleration, 4.0f, 80.0f))
    {
        playerController.setGroundAcceleration(groundAcceleration);
    }
    float airAcceleration = playerController.airAcceleration();
    if (ImGui::SliderFloat("Air Accel", &airAcceleration, 1.0f, 30.0f))
    {
        playerController.setAirAcceleration(airAcceleration);
    }
    float groundFriction = playerController.groundFriction();
    if (ImGui::SliderFloat("Ground Friction", &groundFriction, 0.0f, 30.0f))
    {
        playerController.setGroundFriction(groundFriction);
    }
    float airControl = playerController.airControl();
    if (ImGui::SliderFloat("Air Control", &airControl, 0.0f, 1.0f))
    {
        playerController.setAirControl(airControl);
    }
    float simulationHz = playerController.simulationHz();
    if (ImGui::SliderFloat("Simulation Hz", &simulationHz, 30.0f, 240.0f))
    {
        playerController.setSimulationHz(simulationHz);
    }
    float mouseSensitivity = playerController.mouseSensitivity();
    if (ImGui::SliderFloat("Look Sensitivity", &mouseSensitivity, 0.03f, 0.18f))
    {
        playerController.setMouseSensitivity(mouseSensitivity);
    }
    float crouchSpeedMultiplier = playerController.crouchSpeedMultiplier();
    if (ImGui::SliderFloat("Crouch Speed Mult", &crouchSpeedMultiplier, 0.1f, 1.0f))
    {
        playerController.setCrouchSpeedMultiplier(crouchSpeedMultiplier);
    }
    float stopSpeed = playerController.stopSpeed();
    if (ImGui::SliderFloat("Stop Speed", &stopSpeed, 0.1f, 24.0f))
    {
        playerController.setStopSpeed(stopSpeed);
    }
    float capsuleRadius = playerController.capsuleRadius();
    if (ImGui::SliderFloat("Capsule Radius", &capsuleRadius, 0.1f, 0.8f))
    {
        playerController.setCapsuleRadius(capsuleRadius);
    }
    float standingHeight = playerController.standingHeight();
    if (ImGui::SliderFloat("Standing Height", &standingHeight, 0.8f, 2.6f))
    {
        playerController.setStandingHeight(standingHeight);
    }
    float crouchingHeight = playerController.crouchingHeight();
    if (ImGui::SliderFloat("Crouching Height", &crouchingHeight, 0.6f, 2.2f))
    {
        playerController.setCrouchingHeight(crouchingHeight);
    }
    float standingEyeHeight = playerController.standingEyeHeight();
    if (ImGui::SliderFloat("Standing Eye Height", &standingEyeHeight, 0.2f, 2.4f))
    {
        playerController.setStandingEyeHeight(standingEyeHeight);
    }
    float crouchedEyeHeight = playerController.crouchedEyeHeight();
    if (ImGui::SliderFloat("Crouched Eye Height", &crouchedEyeHeight, 0.2f, 2.0f))
    {
        playerController.setCrouchedEyeHeight(crouchedEyeHeight);
    }
    float stepHeight = playerController.stepHeight();
    if (ImGui::SliderFloat("Step Height", &stepHeight, 0.0f, 1.2f))
    {
        playerController.setStepHeight(stepHeight);
    }
    float supportProbeDistance = playerController.supportProbeDistance();
    if (ImGui::SliderFloat("Support Probe", &supportProbeDistance, 0.01f, 0.5f))
    {
        playerController.setSupportProbeDistance(supportProbeDistance);
    }
    float maxSlopeAngle = playerController.maxSlopeAngleDegrees();
    if (ImGui::SliderFloat("Max Slope", &maxSlopeAngle, 0.0f, 89.0f))
    {
        playerController.setMaxSlopeAngleDegrees(maxSlopeAngle);
    }
    float sweepSkinWidth = playerController.sweepSkinWidth();
    if (ImGui::SliderFloat("Sweep Skin", &sweepSkinWidth, 0.001f, 0.1f, "%.3f"))
    {
        playerController.setSweepSkinWidth(sweepSkinWidth);
    }
    float coyoteTimeSeconds = playerController.coyoteTimeSeconds();
    if (ImGui::SliderFloat("Coyote Time", &coyoteTimeSeconds, 0.0f, 0.5f, "%.3f"))
    {
        playerController.setCoyoteTimeSeconds(coyoteTimeSeconds);
    }
    float jumpBufferSeconds = playerController.jumpBufferSeconds();
    if (ImGui::SliderFloat("Jump Buffer", &jumpBufferSeconds, 0.0f, 0.5f, "%.3f"))
    {
        playerController.setJumpBufferSeconds(jumpBufferSeconds);
    }
    int maxCollisionIterations = playerController.maxCollisionIterations();
    if (ImGui::SliderInt("Max Collision Iterations", &maxCollisionIterations, 1, 12))
    {
        playerController.setMaxCollisionIterations(maxCollisionIterations);
    }
    ImGui::Separator();

    if (ImGui::Button("Capture Mouse"))
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
        ImGui::SliderInt("Terrain Density", &worldSettings.proceduralWorld.terrainDensity, 20, 96);
    regenerateWorld |= ImGui::SliderFloat(
        "Terrain Scale", &worldSettings.proceduralWorld.terrainScale, 24.0f, 96.0f);
    regenerateWorld |= ImGui::SliderFloat(
        "Terrain Height", &worldSettings.proceduralWorld.terrainHeight, 4.0f, 24.0f);
    regenerateWorld |=
        ImGui::SliderInt("Tree Count", &worldSettings.proceduralWorld.treeCount, 0, 180);
    regenerateWorld |=
        ImGui::SliderInt("Generation Seed", &worldSettings.proceduralWorld.seed, 1, 4096);
    if (ImGui::Button("Rebuild World"))
    {
        regenerateWorld = true;
    }

    ImGui::Text("ECS: %d entities / %d component types / %d components", stats.entityCount,
                stats.componentTypeCount, stats.componentCount);
    worldSettings.proceduralWorld.regenerationRequested =
        worldSettings.proceduralWorld.regenerationRequested || regenerateWorld;
    ImGui::End();

    ImGui::Begin("Debug Visibility");

    ImGui::TextUnformatted("Debug Visibility Paths");
    ImGui::Checkbox("Ambient", &renderSettings.debugView.ambientEnabled);
    ImGui::Checkbox("Fog", &renderSettings.debugView.fogEnabled);
    ImGui::Checkbox("Sky Lighting", &renderSettings.debugView.skyLightingEnabled);
    ImGui::Checkbox("Emissive Propagation", &renderSettings.debugView.emissivePropagationEnabled);
    if (ImGui::BeginCombo("Material Debug View",
                          materialDebugModeLabel(renderSettings.debugView.materialDebugViewMode)))
    {
        for (int mode = static_cast<int>(MaterialDebugViewMode::Shaded);
             mode <= static_cast<int>(MaterialDebugViewMode::ShadowFactor); ++mode)
        {
            const bool selected = renderSettings.debugView.materialDebugViewMode == mode;
            if (ImGui::Selectable(materialDebugModeLabel(mode), selected))
            {
                renderSettings.debugView.materialDebugViewMode = mode;
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
    ImGui::Checkbox("Tone Mapping", &renderSettings.debugView.toneMappingEnabled);
    ImGui::Checkbox("Post Processing", &renderSettings.debugView.postProcessingEnabled);
    if (ImGui::BeginCombo("Post Debug View",
                          postDebugModeLabel(renderSettings.debugView.postDebugViewMode)))
    {
        for (int mode = static_cast<int>(PostDebugViewMode::FinalImage);
             mode <= static_cast<int>(PostDebugViewMode::ToneMapped); ++mode)
        {
            const bool selected = renderSettings.debugView.postDebugViewMode == mode;
            if (ImGui::Selectable(postDebugModeLabel(mode), selected))
            {
                renderSettings.debugView.postDebugViewMode = mode;
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
        applyVolumetricPreset(renderSettings, static_cast<VolumetricTuningPreset>(selectedPreset));
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Participating Media");
    ImGui::SliderFloat("Atmospheric Density", &renderSettings.fog.density, 0.0f, 0.2f, "%.4f");
    ImGui::SliderFloat("Height Falloff", &renderSettings.fog.heightFalloff, 0.0f, 4.0f, "%.3f");
    ImGui::SliderFloat("Scattering Strength", &renderSettings.rayEvaluation.scatteringStrength,
                       0.0f, 8.0f, "%.3f");
    ImGui::SliderFloat("Extinction Strength", &renderSettings.rayEvaluation.extinctionStrength,
                       0.0f, 8.0f, "%.3f");
    ImGui::SliderFloat("Phase Anisotropy", &renderSettings.rayEvaluation.phaseAnisotropy, -0.95f,
                       0.95f, "%.3f");
    ImGui::SliderFloat("Atmosphere Ambient Floor",
                       &renderSettings.rayEvaluation.atmosphericAmbientFloor, 0.0f, 0.2f, "%.4f");
    ImGui::SliderFloat("Fog Base Height", &renderSettings.fog.baseHeight, -16.0f, 32.0f, "%.2f");
    ImGui::SliderFloat("Fog Max Height", &renderSettings.fog.maxHeight, 8.0f, 240.0f, "%.1f");
    ImGui::SliderFloat("Near Field Haze", &renderSettings.rayEvaluation.nearFieldHaze, 0.0f, 1.0f,
                       "%.3f");

    ImGui::Separator();
    ImGui::TextUnformatted("Lighting Response");
    ImGui::SliderFloat("Light Intensity Multiplier",
                       &renderSettings.rayEvaluation.volumetricLightIntensity, 0.0f, 16.0f, "%.3f");
    ImGui::SliderFloat("Directional Angular Radius",
                       &renderSettings.rayEvaluation.directionalLightAngularRadius, 0.0f, 0.02f,
                       "%.4f rad");
    ImGui::SliderFloat("Shadow Strength", &renderSettings.rayEvaluation.shadowStrength, 0.0f, 1.0f,
                       "%.3f");

    ImGui::Separator();
    ImGui::TextUnformatted("Raymarch Integration");
    ImGui::SliderInt("Raymarch Step Count", &renderSettings.rayEvaluation.maxSteps, 8, 256);
    ImGui::SliderFloat("Raymarch Step Length", &renderSettings.rayEvaluation.stepLength, 0.1f,
                       10.0f, "%.3f");
    ImGui::SliderFloat("Max March Distance", &renderSettings.rayEvaluation.maxDistance, 16.0f,
                       1500.0f, "%.1f");
    ImGui::SliderFloat("Step Distribution", &renderSettings.rayEvaluation.stepDistributionExponent,
                       0.35f, 3.0f, "%.3f");
    ImGui::SliderFloat("Jitter Strength", &renderSettings.rayEvaluation.jitterStrength, 0.0f, 1.0f,
                       "%.3f");
    ImGui::SliderFloat("Temporal Blend", &renderSettings.rayEvaluation.temporalBlend, 0.0f, 0.98f,
                       "%.3f");
    ImGui::SliderFloat("Depth Reject", &renderSettings.rayEvaluation.temporalDepthThreshold, 0.05f,
                       4.0f, "%.3f");
    ImGui::SliderFloat("Normal Reject", &renderSettings.rayEvaluation.temporalNormalThreshold, 0.1f,
                       0.99f, "%.3f");
    ImGui::SliderFloat("Velocity Reject", &renderSettings.rayEvaluation.temporalVelocityThreshold,
                       1.0f, 32.0f, "%.2f px");
    ImGui::SliderFloat("Bilateral Depth", &renderSettings.rayEvaluation.bilateralDepthFactor, 1.0f,
                       64.0f, "%.2f");

    ImGui::Separator();
    ImGui::TextUnformatted("Display Response");
    ImGui::SliderFloat("Exposure", &renderSettings.postProcess.exposure, 0.1f, 5.0f, "%.3f");
    ImGui::SliderFloat("Bloom Threshold", &renderSettings.postProcess.bloomThreshold, 0.0f, 10.0f,
                       "%.3f");
    ImGui::SliderFloat("Bloom Intensity", &renderSettings.postProcess.bloomIntensity, 0.0f, 3.0f,
                       "%.3f");
    ImGui::SliderFloat("Contrast", &renderSettings.postProcess.contrast, 0.8f, 1.4f, "%.3f");
    ImGui::SliderFloat("Saturation", &renderSettings.postProcess.saturation, 0.0f, 1.25f, "%.3f");
    ImGui::SliderFloat("Midtone Lift", &renderSettings.postProcess.midtoneLift, 0.0f, 0.12f,
                       "%.3f");
    ImGui::SliderFloat("Vignette", &renderSettings.postProcess.vignetteStrength, 0.0f, 0.5f,
                       "%.3f");

    ImGui::Separator();
    ImGui::TextUnformatted("Debug Visualization");
    if (ImGui::BeginCombo(
            "Volumetric Debug View",
            volumetricDebugModeLabel(renderSettings.debugView.volumetricDebugViewMode)))
    {
        for (int mode = static_cast<int>(VolumetricDebugViewMode::FinalImage);
             mode <= static_cast<int>(VolumetricDebugViewMode::TemporalAccumulationConfidence);
             ++mode)
        {
            const bool selected = renderSettings.debugView.volumetricDebugViewMode == mode;
            if (ImGui::Selectable(volumetricDebugModeLabel(mode), selected))
            {
                renderSettings.debugView.volumetricDebugViewMode = mode;
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    renderSettings.rayEvaluation.phaseAnisotropy =
        std::clamp(renderSettings.rayEvaluation.phaseAnisotropy, -0.95f, 0.95f);

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
