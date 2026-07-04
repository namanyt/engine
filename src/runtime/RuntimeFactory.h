#pragma once

#include "runtime/RuntimeIds.h"

#include <functional>
#include <memory>
#include <unordered_map>

namespace engine
{
class RuntimeMode;

/// Factory function type that constructs a new `RuntimeMode` instance.
using RuntimeFactoryFn = std::function<std::unique_ptr<RuntimeMode>()>;

/**
 * @brief Registration-based factory for creating `RuntimeMode` instances by ID.
 *
 * Replaces a switch-case dispatch with a type-erased registration map.
 * Call `registerRuntime()` once per mode (typically at startup), then use
 * `create()` to instantiate the desired runtime.
 *
 * @par Example
 * @code
 * RuntimeFactory::registerRuntime(RuntimeId::Menu, [] {
 *     return std::make_unique<MenuRuntime>();
 * });
 * auto mode = RuntimeFactory::create(RuntimeId::Menu);
 * @endcode
 *
 * @see RuntimeId
 * @see RuntimeMode
 */
class RuntimeFactory final
{
  public:
    /// @brief Register a factory function for the given runtime ID.
    /// @param runtimeId Unique identifier for this runtime mode.
    /// @param factory Callable that returns a new `RuntimeMode` instance.
    static void registerRuntime(RuntimeId runtimeId, RuntimeFactoryFn factory);

    /// @brief Create a new `RuntimeMode` instance for the given ID.
    /// @param runtimeId The registered runtime identifier.
    /// @return A unique_ptr to the constructed runtime mode.
    /// @throws std::runtime_error if no factory is registered for the ID.
    static std::unique_ptr<RuntimeMode> create(RuntimeId runtimeId);

  private:
    static std::unordered_map<RuntimeId, RuntimeFactoryFn>& registry();
};
} // namespace engine
