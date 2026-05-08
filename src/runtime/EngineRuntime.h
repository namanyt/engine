#pragma once

#include "Application.h"
#include "core/RenderPipeline.h"
#include "core/Renderer.h"

#include <memory>

namespace engine
{
class RuntimeMode;

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
class DebugUi;
#endif

class EngineRuntime final
{
  public:
    EngineRuntime();
    ~EngineRuntime();

    EngineRuntime(const EngineRuntime&) = delete;
    EngineRuntime& operator=(const EngineRuntime&) = delete;
    EngineRuntime(EngineRuntime&&) = delete;
    EngineRuntime& operator=(EngineRuntime&&) = delete;

    int run();

  private:
    void activateInitialRuntimeMode();
    void activateRuntimeMode(std::unique_ptr<RuntimeMode> runtimeMode);
    void processRuntimeTransition();

    Application m_application;
    Renderer m_renderer;
    RenderPipeline m_renderPipeline;
#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    std::unique_ptr<DebugUi> m_debugUi;
#endif
    std::unique_ptr<RuntimeMode> m_activeRuntimeMode;
};
} // namespace engine
