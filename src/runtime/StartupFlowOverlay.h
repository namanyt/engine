#pragma once

#include "core/RuntimeOverlay.h"
#include "runtime/SettingsPage.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace engine
{
class AssetManager;
class Renderer;

struct BootSequenceTextEntry final
{
    std::string text;
    Color color = Color::white();
    bool hasColor = false;
};

enum class MainMenuSelection
{
    None,
    NewGame,
    Settings,
    Quit,
};

enum class PauseMenuSelection
{
    None,
    Resume,
    Settings,
    ReturnToMainMenu,
};

struct VisualNovelOverlayPortrait final
{
    std::filesystem::path assetPath;
    float centerXNormalized = 0.5f;
    float baselineYNormalized = 0.94f;
    float nativeScale = 1.0f;
};

struct VisualNovelOverlayModel final
{
    std::filesystem::path backgroundAssetPath;
    std::vector<VisualNovelOverlayPortrait> portraits;
    bool showDialogueChrome = false;
    std::string speakerName;
    std::string dialogueText;
    std::string advancePrompt;
};

class StartupFlowOverlay final
{
  public:
    StartupFlowOverlay() = default;
    StartupFlowOverlay(unsigned int textureId, int width, int height) noexcept;
    ~StartupFlowOverlay();

    StartupFlowOverlay(const StartupFlowOverlay&) = delete;
    StartupFlowOverlay& operator=(const StartupFlowOverlay&) = delete;
    StartupFlowOverlay(StartupFlowOverlay&& other) noexcept;
    StartupFlowOverlay& operator=(StartupFlowOverlay&& other) noexcept;

    bool valid() const noexcept;
    unsigned int textureId() const noexcept;
    int width() const noexcept;
    int height() const noexcept;

    void apply(Renderer& renderer, const RuntimeOverlayOptions& options) const;
    void reset() noexcept;

    static StartupFlowOverlay createSolid(std::uint8_t red, std::uint8_t green, std::uint8_t blue,
                                          std::uint8_t alpha);
    static StartupFlowOverlay createDisclaimer();
    static StartupFlowOverlay createLoadingProgress(const std::string& loadingLabel, int percent,
                                                    const std::string& phase);
    static StartupFlowOverlay createBootSequence(const std::vector<BootSequenceTextEntry>& lines,
                                                 bool showCursor,
                                                 const BootSequenceTextEntry& statusText);
    static StartupFlowOverlay createMenu(const AssetManager& assetManager,
                                         MainMenuSelection selection);
    static StartupFlowOverlay createPauseMenu(PauseMenuSelection selection);
    static StartupFlowOverlay createInteractionPromptTexture(const std::string& promptText);
    static StartupFlowOverlay createSettingsBase(const AssetManager& assetManager,
                                                 const SettingsOverlayViewModel& viewModel);
    static StartupFlowOverlay createSettingsContent(const SettingsOverlayViewModel& viewModel);
    static StartupFlowOverlay createSettingsMenu(const AssetManager& assetManager,
                                                 const SettingsOverlayViewModel& viewModel);
    static StartupFlowOverlay createVisualNovelScene(const AssetManager& assetManager,
                                                     const VisualNovelOverlayModel& viewModel);
    static StartupFlowOverlay
    createVisualNovelDialogueLayer(const VisualNovelOverlayModel& viewModel);

    unsigned int m_textureId = 0;
    int m_width = 0;
    int m_height = 0;
};
} // namespace engine
