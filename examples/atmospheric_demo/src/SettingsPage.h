#pragma once

#include "Application.h"

#include "OverlayUiLayout.h"

#include <string>
#include <vector>

namespace engine
{
enum class SettingsCategory
{
    Display,
    Audio,
    Controls,
    VN,
    Gameplay,
    Graphics,
};

enum class SettingsHoverTargetType
{
    None,
    Category,
    ResolutionSlider,
    WindowedMode,
    BorderlessMode,
    ExclusiveMode,
    VSyncToggle,
    MasterVolumeSlider,
    MusicVolumeSlider,
    UiVolumeSlider,
    VnVolumeSlider,
    AmbientVolumeSlider,
    SfxVolumeSlider,
    MouseSensitivitySlider,
    TypingSpeedSlider,
    AutoAdvanceToggle,
    ScrollTrack,
    ScrollThumb,
    ApplyButton,
    BackButton,
};

struct SettingsHoverTarget final
{
    SettingsHoverTargetType type = SettingsHoverTargetType::None;
    SettingsCategory category = SettingsCategory::Display;
};

bool operator==(const SettingsHoverTarget& left, const SettingsHoverTarget& right) noexcept;
bool operator!=(const SettingsHoverTarget& left, const SettingsHoverTarget& right) noexcept;

struct SettingsOverlayViewModel final
{
    SettingsHoverTarget hoverTarget{};
    SettingsHoverTargetType draggingSlider = SettingsHoverTargetType::None;
    SettingsCategory activeCategory = SettingsCategory::Display;
    int resolutionIndex = 0;
    int resolutionCount = 1;
    int resolutionWidth = 1600;
    int resolutionHeight = 900;
    Application::WindowMode windowMode = Application::WindowMode::Windowed;
    bool vSyncEnabled = false;
    bool autoAdvanceEnabled = false;
    bool applyEnabled = false;
    bool pauseContext = false;
    bool resolutionDragging = false;
    float masterVolume = 0.85f;
    float musicVolume = 0.70f;
    float uiVolume = 1.0f;
    float vnVolume = 1.0f;
    float ambientVolume = 1.0f;
    float sfxVolume = 1.0f;
    float mouseSensitivity = 0.52f;
    float typingSpeed = 0.45f;
    float contentScrollOffset = 0.0f;
};

enum class SettingsPanelItemKind
{
    Section,
    Slider,
    Toggle,
    Segmented,
    Placeholder,
};

struct SettingsSegmentOptionViewModel final
{
    std::wstring label;
    SettingsHoverTarget hoverTarget{};
    bool selected = false;
    overlayui::Rect bounds{};
};

struct SettingsPanelItemViewModel final
{
    SettingsPanelItemKind kind = SettingsPanelItemKind::Placeholder;
    std::wstring label;
    std::wstring description;
    std::wstring valueLabel;
    SettingsHoverTarget hoverTarget{};
    bool enabled = true;
    bool active = false;
    float normalizedValue = 0.0f;
    float topOffset = 0.0f;
    float height = 0.0f;
    overlayui::Rect bounds{};
    overlayui::Rect interactiveBounds{};
    std::vector<SettingsSegmentOptionViewModel> segmentOptions;
};

struct SettingsCategoryEntryViewModel final
{
    SettingsCategory category = SettingsCategory::Display;
    std::wstring title;
    std::wstring subtitle;
    bool active = false;
    bool hovered = false;
    overlayui::Rect bounds{};
};

struct SettingsPageChromeViewModel final
{
    overlayui::Rect shellBounds{};
    overlayui::Rect sidebarBounds{};
    overlayui::Rect contentBounds{};
    overlayui::Rect contentViewportBounds{};
    overlayui::Rect footerBounds{};
    overlayui::Rect scrollTrackBounds{};
    overlayui::Rect scrollThumbBounds{};
    overlayui::Rect backButtonBounds{};
    overlayui::Rect applyButtonBounds{};
};

struct SettingsPageModel final
{
    SettingsCategory activeCategory = SettingsCategory::Display;
    std::wstring activeCategoryTitle;
    std::wstring activeCategorySubtitle;
    float contentHeight = 0.0f;
    SettingsPageChromeViewModel chrome{};
    std::vector<SettingsCategoryEntryViewModel> categories;
    std::vector<SettingsPanelItemViewModel> items;
};

SettingsPageModel buildSettingsPageModel(const SettingsOverlayViewModel& viewModel);

} // namespace engine
