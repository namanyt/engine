#pragma once

#include "runtime/RuntimeIds.h"

#include <functional>
#include <memory>
#include <unordered_map>

namespace engine
{
class RuntimeMode;

using RuntimeFactoryFn = std::function<std::unique_ptr<RuntimeMode>()>;

class RuntimeFactory final
{
  public:
    static void registerRuntime(RuntimeId runtimeId, RuntimeFactoryFn factory);
    static std::unique_ptr<RuntimeMode> create(RuntimeId runtimeId);
    static std::unique_ptr<RuntimeMode>
    createLoadingTransition(const RuntimeTransitionRequest& request);

  private:
    static std::unordered_map<RuntimeId, RuntimeFactoryFn>& registry();
};
} // namespace engine
