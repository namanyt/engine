#include "debug/DebugUi.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

namespace
{
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
    case engine::VolumetricDebugViewMode::Composite:
        return "Composite";
    case engine::VolumetricDebugViewMode::RaySteps:
        return "Ray Steps";
    case engine::VolumetricDebugViewMode::DensityAccumulation:
        return "Density Accumulation";
    case engine::VolumetricDebugViewMode::ScatteringOnly:
        return "Scattering Only";
    case engine::VolumetricDebugViewMode::IntegrationHeatmap:
        return "Integration Heatmap";
    case engine::VolumetricDebugViewMode::SampleCount:
        return "Sample Count";
    }

    return "Unknown";
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
    }

    return "Unknown";
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

void DebugUi::draw(Scene& scene)
{
    if (!m_enabled)
    {
        return;
    }

    ImGui::Begin("Atmospheric Material Lab");

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

    ImGui::TextUnformatted("Debug Visibility Paths");
    ImGui::Checkbox("Ambient", &scene.debugView.ambientEnabled);
    ImGui::Checkbox("Fog", &scene.debugView.fogEnabled);
    ImGui::Checkbox("Sky Lighting", &scene.debugView.skyLightingEnabled);
    ImGui::Checkbox("Emissive Propagation", &scene.debugView.emissivePropagationEnabled);
    if (ImGui::BeginCombo("Material Debug View",
                          materialDebugModeLabel(scene.debugView.materialDebugViewMode)))
    {
        for (int mode = static_cast<int>(MaterialDebugViewMode::Shaded);
             mode <= static_cast<int>(MaterialDebugViewMode::LuminanceHeatmap); ++mode)
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
    if (ImGui::BeginCombo("Volumetric Debug View",
                          volumetricDebugModeLabel(scene.debugView.volumetricDebugViewMode)))
    {
        for (int mode = static_cast<int>(VolumetricDebugViewMode::Composite);
             mode <= static_cast<int>(VolumetricDebugViewMode::SampleCount); ++mode)
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
    ImGui::Separator();

    ImGui::TextUnformatted("Atmosphere / Post");
    ImGui::SliderFloat("Fog Density", &scene.fog.density, 0.0f, 0.08f);
    ImGui::SliderFloat("Fog Height Falloff", &scene.fog.heightFalloff, 0.0f, 0.6f);
    ImGui::SliderFloat("Atmosphere Intensity", &scene.rayEvaluation.atmosphereIntensity, 0.0f,
                       0.95f);
    ImGui::SliderFloat("Extinction", &scene.rayEvaluation.extinction, 0.0f, 1.1f);
    ImGui::SliderFloat("Near Field Haze", &scene.rayEvaluation.nearFieldHaze, 0.0f, 0.8f);
    ImGui::SliderFloat("Emissive Diffusion", &scene.rayEvaluation.emissiveScatter, 0.0f, 2.5f);
    ImGui::SliderFloat("Phase Anisotropy", &scene.rayEvaluation.phaseAnisotropy, -0.2f, 0.82f);
    ImGui::SliderFloat("Jitter Strength", &scene.rayEvaluation.jitterStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Step Distribution", &scene.rayEvaluation.stepDistributionExponent, 0.6f,
                       2.2f);
    ImGui::SliderFloat("Step Length", &scene.rayEvaluation.stepLength, 0.4f, 4.0f);
    ImGui::SliderFloat("Exposure", &scene.postProcess.exposure, 0.05f, 2.0f);
    ImGui::SliderFloat("Bloom Threshold", &scene.postProcess.bloomThreshold, 0.5f, 4.0f);
    ImGui::SliderFloat("Bloom Intensity", &scene.postProcess.bloomIntensity, 0.0f, 0.3f);
    ImGui::SliderFloat("Contrast", &scene.postProcess.contrast, 0.8f, 1.4f);
    ImGui::SliderFloat("Saturation", &scene.postProcess.saturation, 0.0f, 1.25f);
    ImGui::SliderFloat("Midtone Lift", &scene.postProcess.midtoneLift, 0.0f, 0.04f);
    ImGui::SliderFloat("Vignette", &scene.postProcess.vignetteStrength, 0.0f, 0.3f);
    ImGui::Separator();

    if (ImGui::Button("Resume Camera"))
    {
        m_shouldResumeCamera = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("Quit Game"))
    {
        m_shouldQuit = true;
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
