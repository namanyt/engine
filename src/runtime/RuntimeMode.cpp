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

void RuntimeMode::requestTransition(std::unique_ptr<RuntimeMode> nextMode)
{
    m_requestedTransition = std::move(nextMode);
}
} // namespace engine
