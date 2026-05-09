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
    case RuntimeId::VNPrototype:
        return std::make_unique<VNRuntime>(std::filesystem::path{kPrototypeVnScriptPath},
                                           kPrototypeReturnRuntimeId);
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

    std::unique_ptr<RuntimeMode> nextRuntimeMode = request.targetId == RuntimeId::VNPrototype
                                                       ? createVisualNovelRuntime(request)
                                                       : create(request.targetId);

    return std::make_unique<LoadingRuntime>(std::move(nextRuntimeMode),
                                            request.minimumDurationSeconds, std::move(loadingLabel),
                                            request.loadingScreenStyle);
}
} // namespace engine
