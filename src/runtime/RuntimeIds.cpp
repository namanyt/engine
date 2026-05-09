#include "runtime/RuntimeIds.h"

namespace engine
{
const char* runtimeName(RuntimeId runtimeId) noexcept
{
    switch (runtimeId)
    {
    case RuntimeId::Menu:
        return "MenuRuntime";
    case RuntimeId::FoggyTestWorld:
        return "ExplorationRuntime";
    case RuntimeId::DaylightSandbox:
        return "DaylightSandboxRuntime";
    case RuntimeId::VNPrototype:
        return "VNRuntime";
    }

    return "UnknownRuntime";
}

const char* runtimeDisplayName(RuntimeId runtimeId) noexcept
{
    switch (runtimeId)
    {
    case RuntimeId::Menu:
        return "Main Menu";
    case RuntimeId::FoggyTestWorld:
        return "Foggy TestWorld";
    case RuntimeId::DaylightSandbox:
        return "Daylight Sandbox";
    case RuntimeId::VNPrototype:
        return "VN Prototype";
    }

    return "Unknown Runtime";
}
} // namespace engine
