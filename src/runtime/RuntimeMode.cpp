#include "runtime/RuntimeMode.h"

namespace engine
{
RuntimeMode::~RuntimeMode() = default;

void RuntimeMode::prepareActivation(ActivationContext& activationContext)
{
    (void)activationContext;
}

void RuntimeMode::deactivate(Renderer& renderer)
{
    (void)renderer;
}

std::unique_ptr<RuntimeMode> RuntimeMode::consumeRequestedTransition()
{
    return std::move(m_requestedTransition);
}

std::optional<RuntimeTransitionRequest> RuntimeMode::consumeRequestedRuntimeChange()
{
    std::optional<RuntimeTransitionRequest> request = std::move(m_requestedRuntimeChange);
    m_requestedRuntimeChange.reset();
    return request;
}

void RuntimeMode::requestTransition(std::unique_ptr<RuntimeMode> nextMode)
{
    m_requestedTransition = std::move(nextMode);
}

void RuntimeMode::requestRuntimeChange(RuntimeTransitionRequest request)
{
    m_requestedRuntimeChange = std::move(request);
}
} // namespace engine
