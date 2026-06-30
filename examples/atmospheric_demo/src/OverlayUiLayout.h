#pragma once

#include "math/Types.h"

#include <algorithm>

namespace engine::overlayui
{
struct Rect final
{
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

constexpr float kDesignWidth = 1920.0f;
constexpr float kDesignHeight = 1080.0f;

constexpr Rect kMenuNewGameRect{180.0f, 468.0f, 620.0f, 536.0f};
constexpr Rect kMenuSettingsRect{180.0f, 644.0f, 620.0f, 708.0f};
constexpr Rect kMenuQuitRect{180.0f, 732.0f, 620.0f, 796.0f};

constexpr Rect kPauseResumeRect{620.0f, 396.0f, 1200.0f, 450.0f};
constexpr Rect kPauseSettingsRect{620.0f, 486.0f, 1200.0f, 540.0f};
constexpr Rect kPauseReturnRect{620.0f, 576.0f, 1240.0f, 640.0f};

constexpr Rect kSettingsResolutionSliderRect{760.0f, 392.0f, 1260.0f, 424.0f};
constexpr Rect kSettingsWindowedModeRect{580.0f, 490.0f, 780.0f, 548.0f};
constexpr Rect kSettingsBorderlessModeRect{800.0f, 490.0f, 1080.0f, 548.0f};
constexpr Rect kSettingsExclusiveModeRect{1100.0f, 490.0f, 1380.0f, 548.0f};
constexpr Rect kSettingsVSyncRect{580.0f, 620.0f, 760.0f, 676.0f};
constexpr Rect kSettingsBackRect{950.0f, 734.0f, 1160.0f, 794.0f};
constexpr Rect kSettingsApplyRect{1188.0f, 734.0f, 1400.0f, 794.0f};

inline bool contains(const Rect& rect, const Vec2& point) noexcept
{
    return point.x >= rect.left && point.x <= rect.right && point.y >= rect.top &&
           point.y <= rect.bottom;
}

inline Vec2 toDesignSpace(const Vec2& mousePosition, const Vec2& windowSize) noexcept
{
    const float safeWidth = std::max(windowSize.x, 1.0f);
    const float safeHeight = std::max(windowSize.y, 1.0f);
    return Vec2{(mousePosition.x / safeWidth) * kDesignWidth,
                (mousePosition.y / safeHeight) * kDesignHeight};
}

inline float normalizedT(const float value, const float minimum, const float maximum) noexcept
{
    if (maximum <= minimum)
    {
        return 0.0f;
    }

    return std::clamp((value - minimum) / (maximum - minimum), 0.0f, 1.0f);
}
} // namespace engine::overlayui
