#pragma once

#include "runtime/RuntimeIds.h"

#include <memory>

namespace engine
{
class RuntimeMode;

class RuntimeFactory final
{
  public:
    static std::unique_ptr<RuntimeMode> create(RuntimeId runtimeId);
    static std::unique_ptr<RuntimeMode>
    createLoadingTransition(const RuntimeTransitionRequest& request);
};
} // namespace engine
