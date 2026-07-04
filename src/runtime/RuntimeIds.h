#pragma once

#include <filesystem>
#include <string>

namespace engine
{
/// Loading screen presentation style during runtime transitions.
enum class LoadingScreenStyle
{
    ProgressOnly,           ///< Show only a progress bar.
    Disclaimer,             ///< Show a disclaimer text overlay.
    DisclaimerBootSequence, ///< Show disclaimer followed by boot sequence log.
};

/// Unique identifier for each registered runtime mode.
/// @see RuntimeFactory::registerRuntime()
enum class RuntimeId
{
    Menu,            ///< Main menu runtime.
    FoggyTestWorld,  ///< Atmospheric fog test scene.
    DaylightSandbox, ///< Daytime lighting sandbox.
    VNPrototype,     ///< Visual novel prototype runtime.
};

/// Describes a requested transition from the current runtime to another.
struct RuntimeTransitionRequest final
{
    /// Target runtime to activate after the transition completes.
    RuntimeId targetId = RuntimeId::Menu;
    /// Minimum time (seconds) the loading screen must be visible before transitioning.
    float minimumDurationSeconds = 1.6f;
    /// Label shown on the loading screen.
    std::string loadingLabel;
    /// Presentation style for the loading screen.
    LoadingScreenStyle loadingScreenStyle = LoadingScreenStyle::ProgressOnly;
    /// Optional path to a script asset to load during transition.
    std::filesystem::path scriptAssetPath;
    /// Runtime to return to if the target requests a back-navigation.
    RuntimeId returnTargetId = RuntimeId::DaylightSandbox;
};

/// @brief Returns the internal name string for a runtime ID.
/// @param runtimeId The runtime identifier.
/// @return Null-terminated internal name (e.g. "menu", "foggy_test_world").
const char* runtimeName(RuntimeId runtimeId) noexcept;

/// @brief Returns the human-readable display name for a runtime ID.
/// @param runtimeId The runtime identifier.
/// @return Null-terminated display name (e.g. "Main Menu", "Foggy Test World").
const char* runtimeDisplayName(RuntimeId runtimeId) noexcept;
} // namespace engine
