#include "runtime/EngineRuntime.h"

#include "core/Log.h"
#include "core/RenderDebug.h"
#include "runtime/RuntimeFactory.h"
#include "runtime/RuntimeMode.h"

#include <cstdlib>
#include <sstream>

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
#include "debug/DebugUi.h"
#endif

namespace engine
{
EngineRuntime::EngineRuntime()
    : m_application(), m_renderer(m_application.assetManager(), m_application.shaderDirectory()),
      m_renderPipeline(m_renderer)
{
#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
    m_debugUi = std::make_unique<DebugUi>(m_application.nativeWindow());
#endif
}

EngineRuntime::~EngineRuntime()
{
    if (m_activeRuntimeMode != nullptr)
    {
        m_activeRuntimeMode->deactivate(m_renderer);
    }
}

int EngineRuntime::run()
{
    activateInitialRuntimeMode();

    m_application.pollEvents();
    static_cast<void>(m_application.consumeRawInputState());
    m_application.primeFrameState();
    Log::info("EngineRuntime", "Entering runtime loop.");

    while (m_application.isRunning())
    {
        m_renderer.profiler().beginFrame();
        m_application.pollEvents();
        m_application.processInput();
        const RawInputState inputState = m_application.consumeRawInputState();
        const float deltaSeconds = m_application.deltaSeconds();
        const float timeSeconds = m_application.timeSeconds();

        RuntimeMode::UpdateContext updateContext{
            m_application,   m_renderer, m_renderPipeline, inputState, deltaSeconds, timeSeconds,
#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
            m_debugUi.get(),
#endif
        };
        m_activeRuntimeMode->update(updateContext);

        m_application.updateWindowTitle(timeSeconds);

        RuntimeMode::RenderContext renderContext{m_renderer, m_renderPipeline,
                                                 m_application.framebufferWidth(),
                                                 m_application.framebufferHeight(), timeSeconds};
        m_activeRuntimeMode->render(renderContext);

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
        if (m_debugUi != nullptr && m_debugUi->isEnabled())
        {
            ScopedRenderDebugGroup uiGroup("UI Pass");
            const auto uiGpuScope = m_renderer.profiler().makeGpuScope("UI Pass");
            m_debugUi->beginFrame();
            const RuntimeMode::DebugUiContext debugUiContext{m_application, *m_debugUi,
                                                             m_renderer.profiler().stats(),
                                                             m_renderer.debugTextures()};
            m_activeRuntimeMode->drawDebugUi(debugUiContext);
            m_debugUi->endFrame();
        }
#endif

        processRuntimeTransition();

        m_renderer.profiler().endFrame();
        m_application.present();
    }

    return EXIT_SUCCESS;
}

void EngineRuntime::activateInitialRuntimeMode()
{
    activateRuntimeMode(RuntimeFactory::create(RuntimeId::Menu));
}

void EngineRuntime::activateRuntimeMode(std::unique_ptr<RuntimeMode> runtimeMode)
{
    if (m_activeRuntimeMode != nullptr)
    {
        m_activeRuntimeMode->deactivate(m_renderer);
    }

    m_application.clearStatusWindowTitle();
    m_activeRuntimeMode = std::move(runtimeMode);

    RuntimeMode::ActivationContext activationContext{m_application,
                                                     m_renderer,
                                                     m_renderPipeline,
                                                     m_application.assetManager(),
                                                     m_renderer.shaderLibrary(),
                                                     m_application.assetRootDirectory(),
                                                     m_application.shaderDirectory()};
    m_activeRuntimeMode->activate(activationContext);

    std::ostringstream stream;
    stream << "Activated runtime mode: " << m_activeRuntimeMode->name() << '.';
    Log::info("EngineRuntime", stream.str());
}

void EngineRuntime::processRuntimeTransition()
{
    if (m_activeRuntimeMode == nullptr)
    {
        return;
    }

    std::unique_ptr<RuntimeMode> requestedRuntimeMode =
        m_activeRuntimeMode->consumeRequestedTransition();
    if (requestedRuntimeMode != nullptr)
    {
        activateRuntimeMode(std::move(requestedRuntimeMode));
        return;
    }

    const std::optional<RuntimeTransitionRequest> requestedRuntimeChange =
        m_activeRuntimeMode->consumeRequestedRuntimeChange();
    if (!requestedRuntimeChange.has_value())
    {
        return;
    }

    activateRuntimeMode(RuntimeFactory::createLoadingTransition(*requestedRuntimeChange));
}
} // namespace engine
