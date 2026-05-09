#include "runtime/RuntimeFactory.h"

#include "runtime/ExplorationRuntime.h"
#include "runtime/LoadingRuntime.h"
#include "runtime/MenuRuntime.h"

#include <stdexcept>

namespace engine
{
std::unique_ptr<RuntimeMode> RuntimeFactory::create(RuntimeId runtimeId)
{
    switch (runtimeId)
    {
    case RuntimeId::Menu:
        return std::make_unique<MenuRuntime>();
    case RuntimeId::FoggyTestWorld:
        return std::make_unique<ExplorationRuntime>(RuntimeId::FoggyTestWorld);
    case RuntimeId::DaylightSandbox:
        return std::make_unique<ExplorationRuntime>(RuntimeId::DaylightSandbox);
    }

    throw std::runtime_error("Unsupported runtime id requested from RuntimeFactory.");
}

std::unique_ptr<RuntimeMode>
RuntimeFactory::createLoadingTransition(const RuntimeTransitionRequest& request)
{
    std::string loadingLabel = request.loadingLabel;
    if (loadingLabel.empty())
    {
        loadingLabel = runtimeDisplayName(request.targetId);
    }

    return std::make_unique<LoadingRuntime>(create(request.targetId),
                                            request.minimumDurationSeconds, std::move(loadingLabel),
                                            request.loadingScreenStyle);
}
} // namespace engine
