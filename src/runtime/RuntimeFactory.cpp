#include "runtime/RuntimeFactory.h"

#include "runtime/ExplorationRuntime.h"
#include "runtime/LoadingRuntime.h"
#include "runtime/MenuRuntime.h"
#include "runtime/VNRuntime.h"

#include <filesystem>
#include <stdexcept>

namespace engine
{
namespace
{
constexpr const char* kPrototypeVnScriptPath = "scripts/test.vnscript";
constexpr RuntimeId kPrototypeReturnRuntimeId = RuntimeId::DaylightSandbox;

std::unique_ptr<RuntimeMode> createVisualNovelRuntime(const RuntimeTransitionRequest& request)
{
    const std::filesystem::path scriptAssetPath =
        request.scriptAssetPath.empty() ? std::filesystem::path{kPrototypeVnScriptPath}
                                         : request.scriptAssetPath;
    return std::make_unique<VNRuntime>(scriptAssetPath, request.returnTargetId);
}
} // namespace

void RuntimeFactory::registerRuntime(RuntimeId runtimeId, RuntimeFactoryFn factory)
{
    registry()[runtimeId] = std::move(factory);
}

std::unique_ptr<RuntimeMode> RuntimeFactory::create(RuntimeId runtimeId)
{
    auto& map = registry();
    auto iterator = map.find(runtimeId);
    if (iterator != map.end())
    {
        return iterator->second();
    }

    throw std::runtime_error("No factory registered for requested runtime id.");
}

std::unordered_map<RuntimeId, RuntimeFactoryFn>& RuntimeFactory::registry()
{
    static std::unordered_map<RuntimeId, RuntimeFactoryFn> map;
    return map;
}

std::unique_ptr<RuntimeMode>
RuntimeFactory::createLoadingTransition(const RuntimeTransitionRequest& request)
{
    std::string loadingLabel = request.loadingLabel;
    if (loadingLabel.empty())
    {
        loadingLabel = runtimeDisplayName(request.targetId);
    }

    std::unique_ptr<RuntimeMode> nextRuntimeMode = request.targetId == RuntimeId::VNPrototype
                                                       ? createVisualNovelRuntime(request)
                                                       : create(request.targetId);

    return std::make_unique<LoadingRuntime>(std::move(nextRuntimeMode),
                                            request.minimumDurationSeconds, std::move(loadingLabel),
                                            request.loadingScreenStyle);
}
} // namespace engine
