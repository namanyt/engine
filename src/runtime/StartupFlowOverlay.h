#pragma once

#include "core/RuntimeOverlay.h"
#include "runtime/SettingsOverlay.h"

#include <cstdint>

namespace engine
{
class Renderer;

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
    static StartupFlowOverlay createMenu(MainMenuSelection selection);
    static StartupFlowOverlay createPauseMenu(PauseMenuSelection selection);
    static StartupFlowOverlay createSettingsMenu(const SettingsOverlayViewModel& viewModel);

    unsigned int m_textureId = 0;
    int m_width = 0;
    int m_height = 0;
};
} // namespace engine
