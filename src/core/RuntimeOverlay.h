#pragma once

namespace engine
{
enum class RuntimeOverlayLayout
{
    CornerBadge,
    FullScreen,
};

struct RuntimeOverlayOptions final
{
    RuntimeOverlayLayout layout = RuntimeOverlayLayout::CornerBadge;
    float opacity = 1.0f;
};
} // namespace engine
