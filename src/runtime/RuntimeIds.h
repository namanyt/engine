#pragma once

#include <string>

namespace engine
{
enum class LoadingScreenStyle
{
    ProgressOnly,
    Disclaimer,
    DisclaimerBootSequence,
};

enum class RuntimeId
{
    Menu,
    FoggyTestWorld,
    DaylightSandbox,
};

struct RuntimeTransitionRequest final
{
    RuntimeId targetId = RuntimeId::Menu;
    float minimumDurationSeconds = 1.6f;
    std::string loadingLabel;
    LoadingScreenStyle loadingScreenStyle = LoadingScreenStyle::ProgressOnly;
};

const char* runtimeName(RuntimeId runtimeId) noexcept;
const char* runtimeDisplayName(RuntimeId runtimeId) noexcept;
} // namespace engine
