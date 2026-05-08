#include "Application.h"

#include "assets/AssetManager.h"
#include "core/Log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <sstream>
#include <stdexcept>

namespace engine
{
Application::Application()
{
    Log::info("Application", "Starting application bootstrap.");

    try
    {
        initializeWindow();
        m_assetRootDirectory = resolveAssetRootDirectory();
        m_shaderDirectory = resolveShaderDirectory();
        m_assetManager = std::make_shared<AssetManager>();
        const std::size_t discoveredAssets = m_assetManager->discover(m_assetRootDirectory);

        {
            std::ostringstream stream;
            stream << "Asset root: " << m_assetRootDirectory.string();
            Log::info("Application", stream.str());
        }

        std::ostringstream stream;
        stream << "Shader directory: " << m_shaderDirectory.string();
        Log::info("Application", stream.str());

        {
            std::ostringstream assetStream;
            assetStream << "Discovered " << discoveredAssets << " new assets at startup ("
                        << m_assetManager->totalCount() << " total registered).";
            Log::info("Application", assetStream.str());
        }
    }
    catch (...)
    {
        shutdown();
        throw;
    }
}

Application::~Application()
{
    Log::info("Application", "Beginning shutdown.");
    shutdown();
}

bool Application::isRunning() const
{
    return m_window != nullptr && !glfwWindowShouldClose(m_window);
}

void Application::initializeWindow()
{
    Log::info("Application", "Initializing GLFW.");

    if (glfwInit() == GLFW_FALSE)
    {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    m_glfwInitialized = true;
    Log::info("Application", "GLFW initialized successfully.");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    {
        std::ostringstream stream;
        stream << "Creating window " << m_windowWidth << 'x' << m_windowHeight
               << " with OpenGL 3.3 core profile.";
        Log::info("Application", stream.str());
    }

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "EngineStarter", nullptr, nullptr);
    if (m_window == nullptr)
    {
        throw std::runtime_error("Failed to create the GLFW window.");
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(0);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, &Application::framebufferSizeCallback);
    glfwSetWindowSizeCallback(m_window, &Application::windowSizeCallback);
    glfwSetCursorPosCallback(m_window, &Application::cursorPositionCallback);
    glfwGetWindowPos(m_window, &m_windowedPosX, &m_windowedPosY);

    Log::info("Application", "Window created and OpenGL context made current.");
    Log::info("Application", "VSync disabled. Frame rate is uncapped.");

    Log::info("Application", "Loading GLAD function pointers.");

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0)
    {
        throw std::runtime_error("Failed to load OpenGL functions with GLAD.");
    }

    Log::info("Application", "GLAD loaded successfully.");

    glfwGetFramebufferSize(m_window, &m_framebufferWidth, &m_framebufferHeight);
    glViewport(0, 0, m_framebufferWidth, m_framebufferHeight);

    {
        std::ostringstream stream;
        stream << "Initial framebuffer size: " << m_framebufferWidth << 'x' << m_framebufferHeight;
        Log::info("Application", stream.str());
    }

    setCursorCaptured(true);

    const auto* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const auto* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const auto* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    {
        std::ostringstream stream;
        stream << "OpenGL vendor: " << (vendor != nullptr ? vendor : "unknown");
        Log::info("Application", stream.str());
    }

    {
        std::ostringstream stream;
        stream << "OpenGL renderer: " << (renderer != nullptr ? renderer : "unknown");
        Log::info("Application", stream.str());
    }

    {
        std::ostringstream stream;
        stream << "OpenGL version: " << (version != nullptr ? version : "unknown");
        Log::info("Application", stream.str());
    }
}

void Application::processInput()
{
    if (m_window == nullptr)
    {
        return;
    }

    const bool fullscreenTogglePressed = glfwGetKey(m_window, GLFW_KEY_F11) == GLFW_PRESS;
    if (fullscreenTogglePressed && !m_previousFullscreenTogglePressed)
    {
        toggleExclusiveFullscreen();
    }
    m_previousFullscreenTogglePressed = fullscreenTogglePressed;
}

void Application::pollEvents() const
{
    glfwPollEvents();
}

void Application::present() const
{
    glfwSwapBuffers(m_window);
}

float Application::timeSeconds() const
{
    return static_cast<float>(glfwGetTime());
}

float Application::deltaSeconds()
{
    const float currentTime = timeSeconds();
    const float deltaTime = currentTime - m_previousFrameTime;
    m_previousFrameTime = currentTime;
    return deltaTime >= 0.0f ? deltaTime : 0.0f;
}

void Application::primeFrameState()
{
    m_previousFrameTime = timeSeconds();
    m_pendingMouseDelta = Vec2{};
}

RawInputState Application::consumeRawInputState()
{
    RawInputState inputState{};
    inputState.cursorCaptured = m_cursorCaptured;
    inputState.mouseDelta = m_pendingMouseDelta;
    updateKeyboardState(inputState);
    m_pendingMouseDelta = Vec2{};
    return inputState;
}

void Application::setCursorCaptured(bool captured)
{
    if (m_window == nullptr)
    {
        return;
    }

    m_cursorCaptured = captured;
    m_pendingMouseDelta = Vec2{};
    m_hasCursorSample = false;

    glfwSetInputMode(m_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void Application::requestQuit()
{
    if (m_window == nullptr)
    {
        return;
    }

    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Application::setStatusWindowTitle(std::string title)
{
    m_statusWindowTitle = std::move(title);
    applyWindowTitle(m_statusWindowTitle);
}

void Application::clearStatusWindowTitle()
{
    m_statusWindowTitle.clear();
}

void Application::updateWindowTitle(float timeSeconds)
{
    if (m_window == nullptr)
    {
        return;
    }

    if (!m_statusWindowTitle.empty())
    {
        applyWindowTitle(m_statusWindowTitle);
        return;
    }

    ++m_framesSinceLastSample;

    const float elapsedSeconds = timeSeconds - m_lastFpsSampleTime;
    if (elapsedSeconds < 0.5f)
    {
        return;
    }

    const float framesPerSecond = static_cast<float>(m_framesSinceLastSample) / elapsedSeconds;

    std::ostringstream titleStream;
    titleStream << "EngineStarter - " << m_windowWidth << 'x' << m_windowHeight
                << " - FPS: " << static_cast<int>(framesPerSecond + 0.5f);

    applyWindowTitle(titleStream.str());

    m_lastFpsSampleTime = timeSeconds;
    m_framesSinceLastSample = 0;
}

void Application::applyWindowTitle(const std::string& title) const
{
    if (m_window == nullptr)
    {
        return;
    }

    glfwSetWindowTitle(m_window, title.c_str());
}

int Application::framebufferWidth() const noexcept
{
    return m_framebufferWidth;
}

int Application::framebufferHeight() const noexcept
{
    return m_framebufferHeight;
}

const std::filesystem::path& Application::assetRootDirectory() const noexcept
{
    return m_assetRootDirectory;
}

const std::filesystem::path& Application::shaderDirectory() const noexcept
{
    return m_shaderDirectory;
}

const std::shared_ptr<AssetManager>& Application::assetManager() const noexcept
{
    return m_assetManager;
}

bool Application::isCursorCaptured() const noexcept
{
    return m_cursorCaptured;
}

GLFWwindow* Application::nativeWindow() const noexcept
{
    return m_window;
}

void Application::toggleExclusiveFullscreen()
{
    if (m_window == nullptr)
    {
        return;
    }

    if (!m_isExclusiveFullscreen)
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (monitor == nullptr)
        {
            Log::warning("Application", "Unable to enter fullscreen: no monitor available.");
            return;
        }

        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
        if (videoMode == nullptr)
        {
            Log::warning("Application", "Unable to enter fullscreen: no video mode available.");
            return;
        }

        glfwGetWindowPos(m_window, &m_windowedPosX, &m_windowedPosY);
        glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);

        glfwSetWindowMonitor(m_window, monitor, 0, 0, videoMode->width, videoMode->height,
                             videoMode->refreshRate);
        m_windowWidth = videoMode->width;
        m_windowHeight = videoMode->height;
        m_isExclusiveFullscreen = true;

        std::ostringstream stream;
        stream << "Entered exclusive fullscreen at " << videoMode->width << 'x' << videoMode->height
               << " @ " << videoMode->refreshRate << " Hz.";
        Log::info("Application", stream.str());
        return;
    }

    glfwSetWindowMonitor(m_window, nullptr, m_windowedPosX, m_windowedPosY, m_windowedWidth,
                         m_windowedHeight, GLFW_DONT_CARE);
    m_windowWidth = m_windowedWidth;
    m_windowHeight = m_windowedHeight;
    m_isExclusiveFullscreen = false;

    std::ostringstream stream;
    stream << "Returned to windowed mode at " << m_windowedWidth << 'x' << m_windowedHeight << '.';
    Log::info("Application", stream.str());
}

void Application::shutdown() noexcept
{
    if (m_window != nullptr)
    {
        Log::info("Application", "Destroying window.");
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    if (m_glfwInitialized)
    {
        Log::info("Application", "Terminating GLFW.");
        glfwTerminate();
        m_glfwInitialized = false;
    }
}

void Application::updateKeyboardState(RawInputState& inputState) const
{
    if (m_window == nullptr)
    {
        return;
    }

    const auto captureButton =
        [](RawButtonState& buttonState, const bool currentDown, bool& previousDown)
    {
        buttonState.down = currentDown;
        buttonState.pressed = currentDown && !previousDown;
        previousDown = currentDown;
    };

    captureButton(inputState.keyEnter, glfwGetKey(m_window, GLFW_KEY_ENTER) == GLFW_PRESS,
                  m_previousEnterPressed);
    captureButton(inputState.keyUpArrow, glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS,
                  m_previousUpArrowPressed);
    captureButton(inputState.keyDownArrow, glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS,
                  m_previousDownArrowPressed);
    inputState.keyW.down = glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS;
    inputState.keyA.down = glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS;
    inputState.keyS.down = glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS;
    inputState.keyD.down = glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS;
    inputState.keyC.down = glfwGetKey(m_window, GLFW_KEY_C) == GLFW_PRESS;
    captureButton(inputState.keySpace, glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS,
                  m_previousSpacePressed);
    inputState.keyLeftControl.down = glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
    inputState.keyLeftShift.down = glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    captureButton(inputState.keyEscape, glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS,
                  m_previousEscapePressed);
    captureButton(inputState.keyF1, glfwGetKey(m_window, GLFW_KEY_F1) == GLFW_PRESS,
                  m_previousF1Pressed);
    captureButton(inputState.keyF2, glfwGetKey(m_window, GLFW_KEY_F2) == GLFW_PRESS,
                  m_previousF2Pressed);
    captureButton(inputState.keyDigit1, glfwGetKey(m_window, GLFW_KEY_1) == GLFW_PRESS,
                  m_previousDigit1Pressed);
    captureButton(inputState.keyDigit2, glfwGetKey(m_window, GLFW_KEY_2) == GLFW_PRESS,
                  m_previousDigit2Pressed);
    captureButton(inputState.keyDigit3, glfwGetKey(m_window, GLFW_KEY_3) == GLFW_PRESS,
                  m_previousDigit3Pressed);
    captureButton(inputState.keyDigit4, glfwGetKey(m_window, GLFW_KEY_4) == GLFW_PRESS,
                  m_previousDigit4Pressed);
    captureButton(inputState.keyDigit5, glfwGetKey(m_window, GLFW_KEY_5) == GLFW_PRESS,
                  m_previousDigit5Pressed);
    captureButton(inputState.keyDigit6, glfwGetKey(m_window, GLFW_KEY_6) == GLFW_PRESS,
                  m_previousDigit6Pressed);
    captureButton(inputState.keyDigit7, glfwGetKey(m_window, GLFW_KEY_7) == GLFW_PRESS,
                  m_previousDigit7Pressed);
    captureButton(inputState.keyDigit8, glfwGetKey(m_window, GLFW_KEY_8) == GLFW_PRESS,
                  m_previousDigit8Pressed);
    captureButton(inputState.keyDigit9, glfwGetKey(m_window, GLFW_KEY_9) == GLFW_PRESS,
                  m_previousDigit9Pressed);
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (application != nullptr)
    {
        application->m_framebufferWidth = width;
        application->m_framebufferHeight = height;

        std::ostringstream stream;
        stream << "Framebuffer resized to " << width << 'x' << height;
        Log::info("Application", stream.str());
    }
}

void Application::windowSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (application == nullptr)
    {
        return;
    }

    application->m_windowWidth = width;
    application->m_windowHeight = height;

    if (!application->m_isExclusiveFullscreen)
    {
        application->m_windowedWidth = width;
        application->m_windowedHeight = height;
        glfwGetWindowPos(window, &application->m_windowedPosX, &application->m_windowedPosY);
    }

    std::ostringstream stream;
    stream << "Window resized to " << width << 'x' << height;
    Log::info("Application", stream.str());
}

void Application::cursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition)
{
    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (application == nullptr)
    {
        return;
    }

    const Vec2 currentCursorPosition{
        static_cast<float>(xPosition),
        static_cast<float>(yPosition),
    };

    if (!application->m_cursorCaptured)
    {
        application->m_lastCursorPosition = currentCursorPosition;
        application->m_hasCursorSample = true;
        return;
    }

    if (application->m_hasCursorSample)
    {
        application->m_pendingMouseDelta =
            application->m_pendingMouseDelta +
            Vec2{
                currentCursorPosition.x - application->m_lastCursorPosition.x,
                application->m_lastCursorPosition.y - currentCursorPosition.y,
            };
    }

    application->m_lastCursorPosition = currentCursorPosition;
    application->m_hasCursorSample = true;
}

std::filesystem::path Application::resolveShaderDirectory()
{
    return resolveAssetRootDirectory() / "shaders";
}

std::filesystem::path Application::resolveAssetRootDirectory()
{
#ifdef ENGINE_SOURCE_DIR
    const std::filesystem::path sourceAssetRoot =
        std::filesystem::path(ENGINE_SOURCE_DIR) / "assets";
    if (std::filesystem::exists(sourceAssetRoot))
    {
        return sourceAssetRoot;
    }
#endif

    const std::filesystem::path runtimeAssetRoot = std::filesystem::current_path() / "assets";
    if (std::filesystem::exists(runtimeAssetRoot))
    {
        return runtimeAssetRoot;
    }

    return runtimeAssetRoot;
}
} // namespace engine
