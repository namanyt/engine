#pragma once

#include "math/Types.h"

#include <filesystem>

struct GLFWwindow;

namespace engine
{
struct InputState final
{
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;
    bool jump = false;
    bool sprint = false;
    bool toggleDebugFreeCamera = false;
    bool toggleMoonLight = false;
    bool toggleSphereLights = false;
    bool toggleConeLights = false;
    bool toggleMoonEmissive = false;
    bool toggleSphereEmissive = false;
    bool toggleConeEmissive = false;
    bool stepMoonBackward = false;
    bool stepMoonForward = false;
    bool toggleMoonMotion = false;
    bool toggleDebugUi = false;
    bool toggleCursorCapture = false;
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
    InputState consumeInputState();
    void primeFrameState();
    void setCursorCaptured(bool captured);
    void requestQuit();
    void updateWindowTitle(float timeSeconds);

    int framebufferWidth() const noexcept;
    int framebufferHeight() const noexcept;
    const std::filesystem::path& shaderDirectory() const noexcept;
    bool isCursorCaptured() const noexcept;
    GLFWwindow* nativeWindow() const noexcept;

  private:
    void initializeWindow();
    void shutdown() noexcept;
    void updateKeyboardState(InputState& inputState) const;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void cursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition);
    static std::filesystem::path resolveShaderDirectory();

    GLFWwindow* m_window = nullptr;
    std::filesystem::path m_shaderDirectory;
    int m_windowWidth = 1600;
    int m_windowHeight = 900;
    int m_framebufferWidth = 1600;
    int m_framebufferHeight = 900;
    float m_lastFpsSampleTime = 0.0f;
    float m_previousFrameTime = 0.0f;
    Vec2 m_pendingMouseDelta{};
    Vec2 m_lastCursorPosition{};
    int m_framesSinceLastSample = 0;
    bool m_glfwInitialized = false;
    bool m_hasCursorSample = false;
    bool m_cursorCaptured = false;
    mutable bool m_previousEscapePressed = false;
    mutable bool m_previousJumpPressed = false;
    mutable bool m_previousDebugFreeCameraPressed = false;
    mutable bool m_previousMoonTogglePressed = false;
    mutable bool m_previousSphereLightsTogglePressed = false;
    mutable bool m_previousConeLightsTogglePressed = false;
    mutable bool m_previousMoonEmissiveTogglePressed = false;
    mutable bool m_previousSphereEmissiveTogglePressed = false;
    mutable bool m_previousConeEmissiveTogglePressed = false;
    mutable bool m_previousMoonBackwardPressed = false;
    mutable bool m_previousMoonForwardPressed = false;
    mutable bool m_previousMoonMotionTogglePressed = false;
    mutable bool m_previousDebugUiTogglePressed = false;
};
} // namespace engine
