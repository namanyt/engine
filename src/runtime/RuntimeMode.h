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

/**
 * @brief Base class for a runtime mode (scene, menu, sandbox, etc.).
 *
 * Each `RuntimeMode` encapsulates a self-contained application experience.
 * Subclasses implement `activate()`, `update()`, `render()`, and optionally
 * `drawDebugUi()` to define the behavior of that mode.
 *
 * Transitions between modes are initiated via `requestTransition()` or
 * `requestRuntimeChange()`. The base class defers activation until the next
 * frame, allowing the outgoing mode to finish cleanly.
 *
 * @par Example
 * @code
 * class MyScene : public RuntimeMode {
 *     void activate(ActivationContext& ctx) override { // setup
 *     }
 *     void update(const UpdateContext& ctx) override { // per-frame logic
 *     }
 *     void render(const RenderContext& ctx) override { // draw calls
 *     }
 * };
 * @endcode
 *
 * @see RuntimeFactory
 * @see RuntimeTransitionRequest
 */
class RuntimeMode
{
  public:
    /// @brief Data passed to `prepareActivation()` and `activate()`.
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

    /// @brief Data passed to `update()` each frame.
    struct UpdateContext final
    {
        Application& application;
        Renderer& renderer;
        RenderPipeline& renderPipeline;
        const RawInputState& inputState;
        float deltaSeconds = 0.0f; ///< Time elapsed since last frame (seconds).
        float timeSeconds = 0.0f;  ///< Total elapsed application time (seconds).
#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
        DebugUi* debugUi = nullptr; ///< Optional ImGui debug UI handle.
#endif
    };

    /// @brief Data passed to `render()` each frame.
    struct RenderContext final
    {
        Renderer& renderer;
        RenderPipeline& renderPipeline;
        int framebufferWidth = 1;  ///< Current framebuffer width in pixels.
        int framebufferHeight = 1; ///< Current framebuffer height in pixels.
        float timeSeconds = 0.0f;  ///< Total elapsed application time (seconds).
    };

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    /// @brief Data passed to `drawDebugUi()` for debug overlay rendering.
    struct DebugUiContext final
    {
        Application& application;
        DebugUi& debugUi;
        const FramePerformanceStats& performanceStats;
        const RendererDebugTextures& debugTextures;
    };
#endif

    /// @brief Virtual destructor for polymorphic deletion.
    virtual ~RuntimeMode();

    /// @brief Returns the internal name of this runtime mode (e.g. "menu").
    /// @return Null-terminated string identifying this mode.
    virtual const char* name() const = 0;

    /// @brief Called before `activate()` to allow pre-warming resources.
    /// @param activationContext Shared engine subsystems and paths.
    /// @note Default implementation does nothing; override if needed.
    virtual void prepareActivation(ActivationContext& activationContext);

    /// @brief Called when this runtime mode becomes active.
    /// @param activationContext Shared engine subsystems and paths.
    /// @note Override to initialize scene-specific state.
    virtual void activate(ActivationContext& activationContext) = 0;

    /// @brief Called when this runtime mode is deactivated.
    /// @param renderer Reference to the renderer for cleanup.
    /// @note Default implementation does nothing; override if needed.
    virtual void deactivate(Renderer& renderer);

    /// @brief Per-frame update logic (input, physics, game logic).
    /// @param updateContext Frame timing, input state, and subsystems.
    virtual void update(const UpdateContext& updateContext) = 0;

    /// @brief Per-frame rendering calls.
    /// @param renderContext Renderer, framebuffer size, and time.
    virtual void render(const RenderContext& renderContext) = 0;

    /// @brief Returns whether this mode can render a loading preview.
    /// @return true if `renderLoadingPreview()` has a meaningful implementation.
    virtual bool canRenderLoadingPreview() const;

    /// @brief Renders a lightweight preview while assets are loading.
    /// @param renderContext Renderer and framebuffer information.
    virtual void renderLoadingPreview(const RenderContext& renderContext);

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    /// @brief Draws debug UI panels (ImGui) for this runtime mode.
    /// @param debugUiContext Debug UI handle, performance stats, and debug textures.
    virtual void drawDebugUi(const DebugUiContext& debugUiContext) = 0;
#endif

    /// @brief Consumes and returns a pending direct-mode transition.
    /// @return The new `RuntimeMode` if one was requested, or nullptr.
    std::unique_ptr<RuntimeMode> consumeRequestedTransition();

    /// @brief Consumes and returns a pending runtime change request.
    /// @return The `RuntimeTransitionRequest` if one was queued, or std::nullopt.
    std::optional<RuntimeTransitionRequest> consumeRequestedRuntimeChange();

  protected:
    /// @brief Queues a direct transition to another `RuntimeMode` instance.
    /// @param nextMode The runtime mode to activate on the next frame.
    void requestTransition(std::unique_ptr<RuntimeMode> nextMode);

    /// @brief Queues a runtime change described by a transition request.
    /// @param request The target runtime ID and loading screen configuration.
    void requestRuntimeChange(RuntimeTransitionRequest request);

  private:
    std::unique_ptr<RuntimeMode> m_requestedTransition;
    std::optional<RuntimeTransitionRequest> m_requestedRuntimeChange;
};
} // namespace engine
