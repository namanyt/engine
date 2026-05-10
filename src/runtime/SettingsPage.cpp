#include "runtime/SettingsPage.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

namespace engine
{
namespace
{
struct CategoryDefinition final
{
    SettingsCategory category = SettingsCategory::Display;
    const wchar_t* title = L"";
    const wchar_t* subtitle = L"";
    const wchar_t* heading = L"";
    const wchar_t* description = L"";
};

constexpr std::array<CategoryDefinition, 6> kCategoryDefinitions{{
    {SettingsCategory::Display, L"Display", L"Window, mode, sync", L"Display",
     L"Configure how Engine presents the runtime on your display."},
    {SettingsCategory::Audio, L"Audio", L"Mix and output", L"Audio",
     L"Stage engine-wide mix controls here before they are wired into runtime systems."},
    {SettingsCategory::Controls, L"Controls", L"Mouse and bindings", L"Controls",
     L"Keep input tuning centralized so rebinding and device support can grow here."},
    {SettingsCategory::VN, L"VN", L"Dialogue pacing", L"Visual Novel",
     L"Dialogue pacing and auto-advance belong here for VNRuntime and future narrative systems."},
    {SettingsCategory::Gameplay, L"Gameplay", L"Rules and accessibility", L"Gameplay",
     L"Reserve space for player-facing runtime rules, assists, and accessibility controls."},
    {SettingsCategory::Graphics, L"Graphics", L"Pipeline quality", L"Graphics",
     L"Future rendering quality and post-processing controls will plug into this page."},
}};

constexpr float kShellMarginX = 72.0f;
constexpr float kShellMarginY = 56.0f;
constexpr float kShellSidebarWidth = 332.0f;
constexpr float kShellPadding = 34.0f;
constexpr float kCategoryHeight = 84.0f;
constexpr float kCategorySpacing = 14.0f;
constexpr float kContentHeaderHeight = 138.0f;
constexpr float kContentFooterHeight = 112.0f;
constexpr float kContentCardSpacing = 18.0f;
constexpr float kSectionHeight = 78.0f;
constexpr float kSliderHeight = 108.0f;
constexpr float kToggleHeight = 92.0f;
constexpr float kSegmentedHeight = 122.0f;
constexpr float kPlaceholderHeight = 128.0f;
constexpr float kScrollTrackWidth = 8.0f;

const CategoryDefinition& categoryDefinition(const SettingsCategory category)
{
    for (const CategoryDefinition& definition : kCategoryDefinitions)
    {
        if (definition.category == category)
        {
            return definition;
        }
    }

    return kCategoryDefinitions.front();
}

std::wstring percentLabel(const float normalizedValue)
{
    const int percent =
        static_cast<int>(std::round(std::clamp(normalizedValue, 0.0f, 1.0f) * 100.0f));
    return std::to_wstring(percent) + L"%";
}

std::wstring mouseSensitivityLabel(const float normalizedValue)
{
    const float sensitivity = 0.35f + std::clamp(normalizedValue, 0.0f, 1.0f) * 1.40f;
    std::wostringstream stream;
    stream.precision(2);
    stream << std::fixed << sensitivity << L"x";
    return stream.str();
}

std::wstring typingSpeedLabel(const float normalizedValue)
{
    const int charactersPerSecond =
        12 + static_cast<int>(std::round(std::clamp(normalizedValue, 0.0f, 1.0f) * 36.0f));
    return std::to_wstring(charactersPerSecond) + L" cps";
}

SettingsPanelItemViewModel makeSection(std::wstring title, std::wstring description,
                                       float topOffset)
{
    SettingsPanelItemViewModel item{};
    item.kind = SettingsPanelItemKind::Section;
    item.label = std::move(title);
    item.description = std::move(description);
    item.topOffset = topOffset;
    item.height = kSectionHeight;
    return item;
}

SettingsPanelItemViewModel
makeSlider(std::wstring label, std::wstring description, std::wstring valueLabel,
           const SettingsHoverTarget hoverTarget, const SettingsHoverTarget currentHoverTarget,
           const float normalizedValue, const float topOffset, const bool active)
{
    SettingsPanelItemViewModel item{};
    item.kind = SettingsPanelItemKind::Slider;
    item.label = std::move(label);
    item.description = std::move(description);
    item.valueLabel = std::move(valueLabel);
    item.hoverTarget = hoverTarget;
    item.active = active || currentHoverTarget == hoverTarget;
    item.normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
    item.topOffset = topOffset;
    item.height = kSliderHeight;
    return item;
}

SettingsPanelItemViewModel makeToggle(std::wstring label, std::wstring description,
                                      std::wstring valueLabel,
                                      const SettingsHoverTarget hoverTarget,
                                      const SettingsHoverTarget currentHoverTarget,
                                      const bool enabled, const float topOffset)
{
    SettingsPanelItemViewModel item{};
    item.kind = SettingsPanelItemKind::Toggle;
    item.label = std::move(label);
    item.description = std::move(description);
    item.valueLabel = std::move(valueLabel);
    item.hoverTarget = hoverTarget;
    item.active = enabled;
    item.enabled = true;
    item.topOffset = topOffset;
    item.height = kToggleHeight;
    if (currentHoverTarget == hoverTarget)
    {
        item.active = enabled;
    }
    return item;
}

SettingsPanelItemViewModel makePlaceholder(std::wstring title, std::wstring description,
                                           std::wstring valueLabel, const float topOffset)
{
    SettingsPanelItemViewModel item{};
    item.kind = SettingsPanelItemKind::Placeholder;
    item.label = std::move(title);
    item.description = std::move(description);
    item.valueLabel = std::move(valueLabel);
    item.topOffset = topOffset;
    item.height = kPlaceholderHeight;
    return item;
}

SettingsPanelItemViewModel makeDisplayModeItem(const SettingsOverlayViewModel& viewModel,
                                               const float topOffset)
{
    SettingsPanelItemViewModel item{};
    item.kind = SettingsPanelItemKind::Segmented;
    item.label = L"Display Mode";
    item.description =
        L"Keep display presentation explicit so windowing behavior stays predictable.";
    item.topOffset = topOffset;
    item.height = kSegmentedHeight;

    item.segmentOptions.push_back(SettingsSegmentOptionViewModel{
        L"Windowed",
        SettingsHoverTarget{SettingsHoverTargetType::WindowedMode, SettingsCategory::Display},
        viewModel.windowMode == Application::WindowMode::Windowed,
        {}});
    item.segmentOptions.push_back(SettingsSegmentOptionViewModel{
        L"Windowed Fullscreen",
        SettingsHoverTarget{SettingsHoverTargetType::BorderlessMode, SettingsCategory::Display},
        viewModel.windowMode == Application::WindowMode::BorderlessFullscreen,
        {}});
    item.segmentOptions.push_back(SettingsSegmentOptionViewModel{
        L"Exclusive Fullscreen",
        SettingsHoverTarget{SettingsHoverTargetType::ExclusiveMode, SettingsCategory::Display},
        viewModel.windowMode == Application::WindowMode::ExclusiveFullscreen,
        {}});
    return item;
}

float appendDisplayItems(const SettingsOverlayViewModel& viewModel,
                         std::vector<SettingsPanelItemViewModel>& items, float topOffset)
{
    items.push_back(makeSection(
        L"Display Surface",
        L"The display page owns real windowing behavior and keeps changes pending until Apply.",
        topOffset));
    topOffset += kSectionHeight + kContentCardSpacing;

    const std::wstring resolutionLabel = std::to_wstring(viewModel.resolutionWidth) + L" x " +
                                         std::to_wstring(viewModel.resolutionHeight);
    items.push_back(makeSlider(
        L"Resolution",
        L"Choose a presentation size. The slider architecture leaves room for richer display modes "
        L"later.",
        resolutionLabel,
        SettingsHoverTarget{SettingsHoverTargetType::ResolutionSlider, SettingsCategory::Display},
        viewModel.hoverTarget,
        viewModel.resolutionCount > 1 ? static_cast<float>(viewModel.resolutionIndex) /
                                            static_cast<float>(viewModel.resolutionCount - 1)
                                      : 0.0f,
        topOffset, viewModel.resolutionDragging));
    topOffset += kSliderHeight + kContentCardSpacing;

    items.push_back(makeDisplayModeItem(viewModel, topOffset));
    topOffset += kSegmentedHeight + kContentCardSpacing;

    items.push_back(makeToggle(
        L"Vertical Sync",
        L"Keep swap timing stable without instantly mutating the live runtime state.",
        viewModel.vSyncEnabled ? L"On" : L"Off",
        SettingsHoverTarget{SettingsHoverTargetType::VSyncToggle, SettingsCategory::Display},
        viewModel.hoverTarget, viewModel.vSyncEnabled, topOffset));
    topOffset += kToggleHeight;
    return topOffset;
}

float appendAudioItems(const SettingsOverlayViewModel& viewModel,
                       std::vector<SettingsPanelItemViewModel>& items, float topOffset)
{
    items.push_back(makeSection(L"Mix Staging",
                                L"These placeholders establish the long-term audio settings "
                                L"surface before runtime binding lands.",
                                topOffset));
    topOffset += kSectionHeight + kContentCardSpacing;

    items.push_back(makeSlider(
        L"Master Volume", L"Overall runtime output level.", percentLabel(viewModel.masterVolume),
        SettingsHoverTarget{SettingsHoverTargetType::MasterVolumeSlider, SettingsCategory::Audio},
        viewModel.hoverTarget, viewModel.masterVolume, topOffset,
        viewModel.draggingSlider == SettingsHoverTargetType::MasterVolumeSlider));
    topOffset += kSliderHeight + kContentCardSpacing;

    items.push_back(makeSlider(
        L"Music Volume", L"Dedicated bed level for score and ambience.",
        percentLabel(viewModel.musicVolume),
        SettingsHoverTarget{SettingsHoverTargetType::MusicVolumeSlider, SettingsCategory::Audio},
        viewModel.hoverTarget, viewModel.musicVolume, topOffset,
        viewModel.draggingSlider == SettingsHoverTargetType::MusicVolumeSlider));
    topOffset += kSliderHeight + kContentCardSpacing;

    items.push_back(makePlaceholder(L"Next",
                                    L"SFX, voice, output device routing, and bus diagnostics can "
                                    L"slot into this page without changing the shell.",
                                    L"Future-ready", topOffset));
    topOffset += kPlaceholderHeight;
    return topOffset;
}

float appendControlsItems(const SettingsOverlayViewModel& viewModel,
                          std::vector<SettingsPanelItemViewModel>& items, float topOffset)
{
    items.push_back(makeSection(L"Input Tuning",
                                L"Place immediate control tuning here now so rebinding and "
                                L"controller support can arrive later without another redesign.",
                                topOffset));
    topOffset += kSectionHeight + kContentCardSpacing;

    items.push_back(
        makeSlider(L"Mouse Sensitivity", L"Pointer scale for runtime look input.",
                   mouseSensitivityLabel(viewModel.mouseSensitivity),
                   SettingsHoverTarget{SettingsHoverTargetType::MouseSensitivitySlider,
                                       SettingsCategory::Controls},
                   viewModel.hoverTarget, viewModel.mouseSensitivity, topOffset,
                   viewModel.draggingSlider == SettingsHoverTargetType::MouseSensitivitySlider));
    topOffset += kSliderHeight + kContentCardSpacing;

    items.push_back(makePlaceholder(
        L"Keybindings",
        L"Key and device remapping will live in this panel once the binding system lands.",
        L"Placeholder", topOffset));
    topOffset += kPlaceholderHeight;
    return topOffset;
}

float appendVnItems(const SettingsOverlayViewModel& viewModel,
                    std::vector<SettingsPanelItemViewModel>& items, float topOffset)
{
    items.push_back(makeSection(L"Narrative Pace",
                                L"VN-specific pacing belongs in the shared settings surface "
                                L"instead of a separate prototype UI.",
                                topOffset));
    topOffset += kSectionHeight + kContentCardSpacing;

    items.push_back(makeSlider(
        L"Typing Speed",
        L"Controls how quickly dialogue reveals when the typewriter effect is active.",
        typingSpeedLabel(viewModel.typingSpeed),
        SettingsHoverTarget{SettingsHoverTargetType::TypingSpeedSlider, SettingsCategory::VN},
        viewModel.hoverTarget, viewModel.typingSpeed, topOffset,
        viewModel.draggingSlider == SettingsHoverTargetType::TypingSpeedSlider));
    topOffset += kSliderHeight + kContentCardSpacing;

    items.push_back(makeToggle(
        L"Auto Advance", L"Placeholder toggle for autonomous dialogue progression.",
        viewModel.autoAdvanceEnabled ? L"Enabled" : L"Disabled",
        SettingsHoverTarget{SettingsHoverTargetType::AutoAdvanceToggle, SettingsCategory::VN},
        viewModel.hoverTarget, viewModel.autoAdvanceEnabled, topOffset));
    topOffset += kToggleHeight;
    return topOffset;
}

float appendGameplayItems(std::vector<SettingsPanelItemViewModel>& items, float topOffset)
{
    items.push_back(makeSection(
        L"Gameplay Space",
        L"This page is reserved for assists, accessibility, and player-facing runtime rules.",
        topOffset));
    topOffset += kSectionHeight + kContentCardSpacing;

    items.push_back(makePlaceholder(L"Reserved",
                                    L"Difficulty modifiers, subtitle options, accessibility "
                                    L"presets, and interaction tuning can expand here naturally.",
                                    L"Future placeholder", topOffset));
    topOffset += kPlaceholderHeight;
    return topOffset;
}

float appendGraphicsItems(std::vector<SettingsPanelItemViewModel>& items, float topOffset)
{
    items.push_back(makeSection(
        L"Renderer Quality",
        L"Graphics controls will live here once pipeline settings become user-facing.", topOffset));
    topOffset += kSectionHeight + kContentCardSpacing;

    items.push_back(
        makePlaceholder(L"Reserved",
                        L"Quality tiers, post-processing toggles, and pipeline diagnostics can "
                        L"plug into this tab without changing navigation or layout.",
                        L"Future placeholder", topOffset));
    topOffset += kPlaceholderHeight;
    return topOffset;
}

overlayui::Rect makeRect(const float left, const float top, const float right, const float bottom)
{
    return overlayui::Rect{left, top, right, bottom};
}

} // namespace

bool operator==(const SettingsHoverTarget& left, const SettingsHoverTarget& right) noexcept
{
    return left.type == right.type && left.category == right.category;
}

bool operator!=(const SettingsHoverTarget& left, const SettingsHoverTarget& right) noexcept
{
    return !(left == right);
}

SettingsPageModel buildSettingsPageModel(const SettingsOverlayViewModel& viewModel)
{
    SettingsPageModel page{};
    page.activeCategory = viewModel.activeCategory;

    const float shellLeft = kShellMarginX;
    const float shellTop = kShellMarginY;
    const float shellRight = overlayui::kDesignWidth - kShellMarginX;
    const float shellBottom = overlayui::kDesignHeight - kShellMarginY;
    page.chrome.shellBounds = makeRect(shellLeft, shellTop, shellRight, shellBottom);
    page.chrome.sidebarBounds =
        makeRect(shellLeft + kShellPadding, shellTop + kShellPadding,
                 shellLeft + kShellPadding + kShellSidebarWidth, shellBottom - kShellPadding);
    page.chrome.contentBounds =
        makeRect(page.chrome.sidebarBounds.right + 28.0f, shellTop + kShellPadding,
                 shellRight - kShellPadding, shellBottom - kShellPadding);
    page.chrome.footerBounds = makeRect(
        page.chrome.contentBounds.left, page.chrome.contentBounds.bottom - kContentFooterHeight,
        page.chrome.contentBounds.right, page.chrome.contentBounds.bottom);
    page.chrome.contentViewportBounds = makeRect(
        page.chrome.contentBounds.left, page.chrome.contentBounds.top + kContentHeaderHeight,
        page.chrome.contentBounds.right - 24.0f, page.chrome.footerBounds.top - 20.0f);
    page.chrome.backButtonBounds =
        makeRect(page.chrome.footerBounds.right - 444.0f, page.chrome.footerBounds.top + 28.0f,
                 page.chrome.footerBounds.right - 232.0f, page.chrome.footerBounds.top + 86.0f);
    page.chrome.applyButtonBounds =
        makeRect(page.chrome.footerBounds.right - 212.0f, page.chrome.footerBounds.top + 28.0f,
                 page.chrome.footerBounds.right, page.chrome.footerBounds.top + 86.0f);
    page.chrome.scrollTrackBounds =
        makeRect(page.chrome.contentViewportBounds.right + 10.0f,
                 page.chrome.contentViewportBounds.top + 6.0f,
                 page.chrome.contentViewportBounds.right + 10.0f + kScrollTrackWidth,
                 page.chrome.contentViewportBounds.bottom - 6.0f);

    float sidebarTop = page.chrome.sidebarBounds.top + 118.0f;
    for (const CategoryDefinition& definition : kCategoryDefinitions)
    {
        SettingsCategoryEntryViewModel categoryEntry{};
        categoryEntry.category = definition.category;
        categoryEntry.title = definition.title;
        categoryEntry.subtitle = definition.subtitle;
        categoryEntry.active = viewModel.activeCategory == definition.category;
        categoryEntry.hovered =
            viewModel.hoverTarget ==
            SettingsHoverTarget{SettingsHoverTargetType::Category, definition.category};
        categoryEntry.bounds =
            makeRect(page.chrome.sidebarBounds.left, sidebarTop, page.chrome.sidebarBounds.right,
                     sidebarTop + kCategoryHeight);
        page.categories.push_back(std::move(categoryEntry));
        sidebarTop += kCategoryHeight + kCategorySpacing;
    }

    const CategoryDefinition& activeDefinition = categoryDefinition(viewModel.activeCategory);
    page.activeCategoryTitle = activeDefinition.heading;
    page.activeCategorySubtitle = activeDefinition.description;

    float contentTop = 0.0f;
    switch (viewModel.activeCategory)
    {
    case SettingsCategory::Display:
        contentTop = appendDisplayItems(viewModel, page.items, contentTop);
        break;
    case SettingsCategory::Audio:
        contentTop = appendAudioItems(viewModel, page.items, contentTop);
        break;
    case SettingsCategory::Controls:
        contentTop = appendControlsItems(viewModel, page.items, contentTop);
        break;
    case SettingsCategory::VN:
        contentTop = appendVnItems(viewModel, page.items, contentTop);
        break;
    case SettingsCategory::Gameplay:
        contentTop = appendGameplayItems(page.items, contentTop);
        break;
    case SettingsCategory::Graphics:
        contentTop = appendGraphicsItems(page.items, contentTop);
        break;
    }
    page.contentHeight = std::max(contentTop, page.chrome.contentViewportBounds.bottom -
                                                  page.chrome.contentViewportBounds.top);

    const float viewportLeft = page.chrome.contentViewportBounds.left;
    const float viewportRight = page.chrome.contentViewportBounds.right;
    const float scrollRange =
        std::max(page.contentHeight - (page.chrome.contentViewportBounds.bottom -
                                       page.chrome.contentViewportBounds.top),
                 0.0f);
    const float clampedScroll = std::clamp(viewModel.contentScrollOffset, 0.0f, scrollRange);
    for (SettingsPanelItemViewModel& item : page.items)
    {
        const float itemTop =
            page.chrome.contentViewportBounds.top + item.topOffset - clampedScroll;
        item.bounds = makeRect(viewportLeft, itemTop, viewportRight, itemTop + item.height);

        if (item.kind == SettingsPanelItemKind::Slider)
        {
            item.interactiveBounds = makeRect(item.bounds.left + 484.0f, item.bounds.top + 56.0f,
                                              item.bounds.right - 48.0f, item.bounds.top + 88.0f);
        }
        else if (item.kind == SettingsPanelItemKind::Toggle)
        {
            item.interactiveBounds = makeRect(item.bounds.right - 208.0f, item.bounds.top + 22.0f,
                                              item.bounds.right - 24.0f, item.bounds.top + 74.0f);
        }
        else if (item.kind == SettingsPanelItemKind::Segmented)
        {
            item.interactiveBounds =
                makeRect(item.bounds.left + 24.0f, item.bounds.top + 58.0f,
                         item.bounds.right - 24.0f, item.bounds.bottom - 20.0f);
            if (!item.segmentOptions.empty())
            {
                const float segmentSpacing = 14.0f;
                const float totalSpacing =
                    segmentSpacing * static_cast<float>(item.segmentOptions.size() - 1);
                const float segmentWidth =
                    (item.interactiveBounds.right - item.interactiveBounds.left - totalSpacing) /
                    static_cast<float>(item.segmentOptions.size());
                float segmentLeft = item.interactiveBounds.left;
                for (SettingsSegmentOptionViewModel& option : item.segmentOptions)
                {
                    option.bounds =
                        makeRect(segmentLeft, item.interactiveBounds.top,
                                 segmentLeft + segmentWidth, item.interactiveBounds.bottom);
                    segmentLeft += segmentWidth + segmentSpacing;
                }
            }
        }
    }

    const float viewportHeight =
        page.chrome.contentViewportBounds.bottom - page.chrome.contentViewportBounds.top;
    const float thumbHeight =
        scrollRange > 0.0f ? std::max(72.0f, viewportHeight * (viewportHeight / page.contentHeight))
                           : viewportHeight - 12.0f;
    const float trackHeight =
        page.chrome.scrollTrackBounds.bottom - page.chrome.scrollTrackBounds.top;
    const float thumbTravel = std::max(trackHeight - thumbHeight, 0.0f);
    const float thumbOffset =
        scrollRange > 0.0f ? (clampedScroll / scrollRange) * thumbTravel : 0.0f;
    page.chrome.scrollThumbBounds = makeRect(
        page.chrome.scrollTrackBounds.left, page.chrome.scrollTrackBounds.top + thumbOffset,
        page.chrome.scrollTrackBounds.right,
        page.chrome.scrollTrackBounds.top + thumbOffset + thumbHeight);

    return page;
}

} // namespace engine
