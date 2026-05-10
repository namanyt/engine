#pragma once

namespace engine
{
enum class RuntimeOverlayLayout
{
    CornerBadge,
    FullScreen,
    Centered,
    CustomPixels,
};

enum class RuntimeOverlayEffect
{
    Standard,
    BackgroundInvert,
};

struct RuntimeOverlayOptions final
{
    RuntimeOverlayLayout layout = RuntimeOverlayLayout::CornerBadge;
    float opacity = 1.0f;
    RuntimeOverlayEffect effect = RuntimeOverlayEffect::Standard;
    float minXPixels = 0.0f;
    float minYPixels = 0.0f;
    float widthPixels = 0.0f;
    float heightPixels = 0.0f;
};
} // namespace engine
