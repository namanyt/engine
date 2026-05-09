#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include "runtime/RuntimeIds.h"

namespace engine
{
class Application;
class AssetManager;
class DebugUi;
class RenderPipeline;
class Renderer;
class ShaderLibrary;
struct FramePerformanceStats;
struct RawInputState;
struct RendererDebugTextures;

class RuntimeMode
{
  public:
    struct ActivationContext final
    {
        Application& application;
        Renderer& renderer;
        RenderPipeline& renderPipeline;
        std::shared_ptr<AssetManager> assetManager;
        ShaderLibrary& shaderLibrary;
        const std::filesystem::path& assetRootDirectory;
        const std::filesystem::path& shaderDirectory;
    };

    struct UpdateContext final
    {
        Application& application;
        Renderer& renderer;
        RenderPipeline& renderPipeline;
        const RawInputState& inputState;
        float deltaSeconds = 0.0f;
        float timeSeconds = 0.0f;
#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
        DebugUi* debugUi = nullptr;
#endif
    };

    struct RenderContext final
    {
        Renderer& renderer;
        RenderPipeline& renderPipeline;
        int framebufferWidth = 1;
        int framebufferHeight = 1;
        float timeSeconds = 0.0f;
    };

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    struct DebugUiContext final
    {
        Application& application;
        DebugUi& debugUi;
        const FramePerformanceStats& performanceStats;
        const RendererDebugTextures& debugTextures;
    };
#endif

    virtual ~RuntimeMode();

    virtual const char* name() const = 0;
    virtual void prepareActivation(ActivationContext& activationContext);
    virtual void activate(ActivationContext& activationContext) = 0;
    virtual void deactivate(Renderer& renderer);
    virtual void update(const UpdateContext& updateContext) = 0;
    virtual void render(const RenderContext& renderContext) = 0;

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    virtual void drawDebugUi(const DebugUiContext& debugUiContext) = 0;
#endif

    std::unique_ptr<RuntimeMode> consumeRequestedTransition();
    std::optional<RuntimeTransitionRequest> consumeRequestedRuntimeChange();

  protected:
    void requestTransition(std::unique_ptr<RuntimeMode> nextMode);
    void requestRuntimeChange(RuntimeTransitionRequest request);

  private:
    std::unique_ptr<RuntimeMode> m_requestedTransition;
    std::optional<RuntimeTransitionRequest> m_requestedRuntimeChange;
};
} // namespace engine
