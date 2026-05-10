#pragma once

#include "core/RuntimeOverlay.h"
#include "math/Transform.h"

#include <filesystem>
#include <memory>
#include <vector>

#include "core/RenderProfiler.h"
#include "world/RayTracing.h"
#include "world/Lighting.h"
#include "world/Material.h"

namespace engine
{
class AssetManager;
class Mesh;
class PostProcessor;
class RayEvaluationPass;
class Shader;
class ShaderLibrary;
class ShadowMapPass;

struct FrameUniforms final
{
    Mat4 viewMatrix = Mat4::identity();
    Mat4 projectionMatrix = Mat4::identity();
    Mat4 inverseViewMatrix = Mat4::identity();
    Mat4 inverseProjectionMatrix = Mat4::identity();
    Mat4 previousInverseProjectionMatrix = Mat4::identity();
    Mat4 viewProjectionMatrix = Mat4::identity();
    Mat4 previousViewProjectionMatrix = Mat4::identity();
    Mat4 lightViewProjectionMatrix = Mat4::identity();
    Vec3 viewPosition{};
    Vec3 previousViewPosition{};
    Vec3 viewForward{0.0f, 0.0f, -1.0f};
    Vec3 previousViewForward{0.0f, 0.0f, -1.0f};
    Vec3 viewRight{1.0f, 0.0f, 0.0f};
    Vec3 viewUp{0.0f, 1.0f, 0.0f};
    Vec3 previousLightDirection{0.0f, -1.0f, 0.0f};
    float timeSeconds = 0.0f;
    int frameIndex = 0;
    float aspectRatio = 1.0f;
    float verticalFieldOfViewRadians = 0.78539816339f;
    float nearPlane = 0.1f;
    Vec3 fogColor{0.18f, 0.24f, 0.30f};
    float fogDensity = 0.02f;
    float fogBaseHeight = 3.0f;
    float fogHeightFalloff = 0.08f;
    float fogMaxHeight = 96.0f;
    DirectionalLight directionalLight{};
    std::vector<LocalLight> localLights{};
    ShadowSettings shadowSettings{};
    SkyLight skyLight{};
    RayEvaluationSettings rayEvaluation{};
    DebugViewSettings debugView{};
    RayTracingScene rayTracingScene{};
    float exposure = 1.10f;
    float bloomThreshold = 1.05f;
};

struct RendererDebugTextures final
{
    unsigned int shadowMapTextureId = 0;
    unsigned int sceneDepthTextureId = 0;
    unsigned int volumetricTextureId = 0;
};

class Renderer final
{
  public:
    Renderer(const std::shared_ptr<AssetManager>& assetManager,
             const std::filesystem::path& shaderDirectory);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void setViewport(int width, int height);
    void prepareOverlayRenderingResources();
    void prepareWorldRenderingResources();
    void beginFrame(const Color& clearColor);
    void endFrame(const PostProcessSettings& postProcessSettings,
                  const FrameUniforms& frameUniforms, float timeSeconds,
                  bool drawRuntimeOverlay = true) const;
    void endOverlayFrame() const;
    void restoreSceneDepthToBackbuffer() const;
    void drawRuntimeOverlayLayers() const;
    void beginShadowPass(const FrameUniforms& frameUniforms) const;
    void drawShadow(const Mesh& mesh, const Mat4& modelMatrix) const;
    void drawShadow(const Mesh& mesh, const Transform& transform) const;
    void endShadowPass() const;
    void draw(const Mesh& mesh, const Material& material, const Mat4& modelMatrix,
              const FrameUniforms& frameUniforms, unsigned int textureId = 0,
              float opacity = 1.0f) const;
    void draw(const Mesh& mesh, const Material& material, const Transform& transform,
              const FrameUniforms& frameUniforms, unsigned int textureId = 0,
              float opacity = 1.0f) const;
    ShaderLibrary& shaderLibrary() noexcept;
    const ShaderLibrary& shaderLibrary() const noexcept;
    RenderProfiler& profiler() noexcept;
    const RenderProfiler& profiler() const noexcept;
    int framebufferWidth() const noexcept;
    int framebufferHeight() const noexcept;
    RendererDebugTextures debugTextures() const noexcept;
    void setRuntimeOverlayTexture(unsigned int textureId, int width, int height);
    void setRuntimeOverlayTexture(unsigned int textureId, int width, int height,
                                  const RuntimeOverlayOptions& options);
    void setSecondaryRuntimeOverlayTexture(unsigned int textureId, int width, int height);
    void setSecondaryRuntimeOverlayTexture(unsigned int textureId, int width, int height,
                                           const RuntimeOverlayOptions& options);
    void clearRuntimeOverlayTexture();
    void clearSecondaryRuntimeOverlayTexture();

  private:
    void applyFrameState(const Shader& shader, const FrameUniforms& frameUniforms) const;
    void applyMaterialState(const Shader& shader, const Material& material) const;
    void applyLocalLightState(const Shader& shader, const FrameUniforms& frameUniforms) const;

    int m_framebufferWidth = 800;
    int m_framebufferHeight = 600;
    std::shared_ptr<ShaderLibrary> m_shaderLibrary;
    std::unique_ptr<PostProcessor> m_postProcessor;
    std::unique_ptr<RayEvaluationPass> m_rayEvaluationPass;
    std::unique_ptr<ShadowMapPass> m_shadowMapPass;
    mutable RenderProfiler m_profiler;
};
} // namespace engine
