#pragma once

namespace engine
{
/// @brief Layout mode for runtime overlay textures.
enum class RuntimeOverlayLayout
{
    CornerBadge,  ///< Small badge anchored to a screen corner.
    FullScreen,   ///< Overlay spans the entire viewport.
    Centered,     ///< Overlay centered on screen.
    CustomPixels, ///< Position and size controlled by explicit pixel values.
};

/// @brief Visual effect applied to the overlay during compositing.
enum class RuntimeOverlayEffect
{
    Standard,         ///< Normal alpha-blended compositing.
    BackgroundInvert, ///< Inverts background colors behind the overlay.
};

/// @brief Configuration for a runtime overlay texture.
struct RuntimeOverlayOptions final
{
    RuntimeOverlayLayout layout = RuntimeOverlayLayout::CornerBadge; ///< Positioning mode.
    float opacity = 1.0f;                                         ///< Alpha multiplier (0.0–1.0).
    RuntimeOverlayEffect effect = RuntimeOverlayEffect::Standard; ///< Compositing effect.
    float minXPixels = 0.0f;   ///< Left edge position (custom layout only).
    float minYPixels = 0.0f;   ///< Top edge position (custom layout only).
    float widthPixels = 0.0f;  ///< Overlay width (custom layout only).
    float heightPixels = 0.0f; ///< Overlay height (custom layout only).
};
} // namespace engine
