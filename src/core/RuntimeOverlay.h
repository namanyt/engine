#pragma once

namespace engine
{
enum class RuntimeOverlayLayout
{
    CornerBadge,
    FullScreen,
    Centered,
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
};
} // namespace engine
