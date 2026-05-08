#pragma once

#include "math/Types.h"

#include <filesystem>
#include <memory>
#include <string>

struct GLFWwindow;

namespace engine
{
class AssetManager;

struct RawButtonState final
{
    bool down = false;
    bool pressed = false;
};

struct RawInputState final
{
    RawButtonState keyEnter{};
    RawButtonState keyUpArrow{};
    RawButtonState keyDownArrow{};
    RawButtonState keyW{};
    RawButtonState keyA{};
    RawButtonState keyS{};
    RawButtonState keyD{};
    RawButtonState keyC{};
    RawButtonState keySpace{};
    RawButtonState keyLeftControl{};
    RawButtonState keyLeftShift{};
    RawButtonState keyEscape{};
    RawButtonState keyF1{};
    RawButtonState keyF2{};
    RawButtonState keyDigit1{};
    RawButtonState keyDigit2{};
    RawButtonState keyDigit3{};
    RawButtonState keyDigit4{};
    RawButtonState keyDigit5{};
    RawButtonState keyDigit6{};
    RawButtonState keyDigit7{};
    RawButtonState keyDigit8{};
    RawButtonState keyDigit9{};
    bool cursorCaptured = false;
    Vec2 mouseDelta{};
};

class Application final
{
  public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    bool isRunning() const;
    void processInput();
    void pollEvents() const;
    void present() const;
    float timeSeconds() const;
    float deltaSeconds();
    RawInputState consumeRawInputState();
    void primeFrameState();
    void setCursorCaptured(bool captured);
    void requestQuit();
    void setStatusWindowTitle(std::string title);
    void clearStatusWindowTitle();
    void updateWindowTitle(float timeSeconds);

    int framebufferWidth() const noexcept;
    int framebufferHeight() const noexcept;
    const std::filesystem::path& assetRootDirectory() const noexcept;
    const std::filesystem::path& shaderDirectory() const noexcept;
    const std::shared_ptr<AssetManager>& assetManager() const noexcept;
    bool isCursorCaptured() const noexcept;
    GLFWwindow* nativeWindow() const noexcept;

  private:
    void initializeWindow();
    void shutdown() noexcept;
    void updateKeyboardState(RawInputState& inputState) const;
    void toggleExclusiveFullscreen();

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void windowSizeCallback(GLFWwindow* window, int width, int height);
    static void cursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition);
    static std::filesystem::path resolveAssetRootDirectory();
    static std::filesystem::path resolveShaderDirectory();
    void applyWindowTitle(const std::string& title) const;

    GLFWwindow* m_window = nullptr;
    std::shared_ptr<AssetManager> m_assetManager;
    std::filesystem::path m_assetRootDirectory;
    std::filesystem::path m_shaderDirectory;
    int m_windowWidth = 1600;
    int m_windowHeight = 900;
    int m_windowedPosX = 100;
    int m_windowedPosY = 100;
    int m_windowedWidth = 1600;
    int m_windowedHeight = 900;
    int m_framebufferWidth = 1600;
    int m_framebufferHeight = 900;
    float m_lastFpsSampleTime = 0.0f;
    float m_previousFrameTime = 0.0f;
    Vec2 m_pendingMouseDelta{};
    Vec2 m_lastCursorPosition{};
    std::string m_statusWindowTitle;
    int m_framesSinceLastSample = 0;
    bool m_glfwInitialized = false;
    bool m_hasCursorSample = false;
    bool m_cursorCaptured = false;
    bool m_isExclusiveFullscreen = false;
    mutable bool m_previousFullscreenTogglePressed = false;
    mutable bool m_previousEnterPressed = false;
    mutable bool m_previousUpArrowPressed = false;
    mutable bool m_previousDownArrowPressed = false;
    mutable bool m_previousEscapePressed = false;
    mutable bool m_previousSpacePressed = false;
    mutable bool m_previousF1Pressed = false;
    mutable bool m_previousF2Pressed = false;
    mutable bool m_previousDigit1Pressed = false;
    mutable bool m_previousDigit2Pressed = false;
    mutable bool m_previousDigit3Pressed = false;
    mutable bool m_previousDigit4Pressed = false;
    mutable bool m_previousDigit5Pressed = false;
    mutable bool m_previousDigit6Pressed = false;
    mutable bool m_previousDigit7Pressed = false;
    mutable bool m_previousDigit8Pressed = false;
    mutable bool m_previousDigit9Pressed = false;
};
} // namespace engine
