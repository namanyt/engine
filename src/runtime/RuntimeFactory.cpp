#include "runtime/RuntimeFactory.h"
#include "runtime/RuntimeMode.h"

#include <stdexcept>

namespace engine
{
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
} // namespace engine
