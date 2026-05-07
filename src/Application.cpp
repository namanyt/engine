#include "Application.h"

#include "core/Log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <sstream>
#include <stdexcept>

namespace
{
bool hasRequiredShaders(const std::filesystem::path& directory)
{
    return std::filesystem::exists(directory / "vertex.glsl") &&
           std::filesystem::exists(directory / "fragment.glsl") &&
           std::filesystem::exists(directory / "post_blur.vert") &&
           std::filesystem::exists(directory / "post_blur.frag") &&
           std::filesystem::exists(directory / "post_compose.frag") &&
           std::filesystem::exists(directory / "post_tonemap.vert") &&
           std::filesystem::exists(directory / "post_tonemap.frag") &&
           std::filesystem::exists(directory / "ray_eval.vert") &&
           std::filesystem::exists(directory / "ray_eval.frag") &&
           std::filesystem::exists(directory / "shadow_depth.vert") &&
           std::filesystem::exists(directory / "shadow_depth.frag");
}
} // namespace

namespace engine
{
Application::Application()
{
    Log::info("Application", "Starting application bootstrap.");

    try
    {
        initializeWindow();
        m_shaderDirectory = resolveShaderDirectory();

        std::ostringstream stream;
        stream << "Shader directory: " << m_shaderDirectory.string();
        Log::info("Application", stream.str());
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
    glfwSetCursorPosCallback(m_window, &Application::cursorPositionCallback);

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

#if !defined(ENGINE_ENABLE_DEBUG_UI) || defined(NDEBUG)
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        Log::info("Application", "Escape pressed, requesting shutdown.");
        requestQuit();
    }
#endif
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

InputState Application::consumeInputState()
{
    InputState inputState{};
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

void Application::updateWindowTitle(float timeSeconds)
{
    if (m_window == nullptr)
    {
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

    glfwSetWindowTitle(m_window, titleStream.str().c_str());

    m_lastFpsSampleTime = timeSeconds;
    m_framesSinceLastSample = 0;
}

int Application::framebufferWidth() const noexcept
{
    return m_framebufferWidth;
}

int Application::framebufferHeight() const noexcept
{
    return m_framebufferHeight;
}

const std::filesystem::path& Application::shaderDirectory() const noexcept
{
    return m_shaderDirectory;
}

bool Application::isCursorCaptured() const noexcept
{
    return m_cursorCaptured;
}

GLFWwindow* Application::nativeWindow() const noexcept
{
    return m_window;
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

void Application::updateKeyboardState(InputState& inputState) const
{
    if (m_window == nullptr)
    {
        return;
    }

    inputState.moveForward = glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS;
    inputState.moveBackward = glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS;
    inputState.moveLeft = glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS;
    inputState.moveRight = glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS;
    inputState.moveUp = glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS;
    inputState.moveDown = glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                          glfwGetKey(m_window, GLFW_KEY_C) == GLFW_PRESS;
    inputState.sprint = glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    const bool moonTogglePressed = glfwGetKey(m_window, GLFW_KEY_1) == GLFW_PRESS;
    inputState.toggleMoonLight = moonTogglePressed && !m_previousMoonTogglePressed;
    m_previousMoonTogglePressed = moonTogglePressed;

    const bool sphereLightsTogglePressed = glfwGetKey(m_window, GLFW_KEY_2) == GLFW_PRESS;
    inputState.toggleSphereLights =
        sphereLightsTogglePressed && !m_previousSphereLightsTogglePressed;
    m_previousSphereLightsTogglePressed = sphereLightsTogglePressed;

    const bool coneLightsTogglePressed = glfwGetKey(m_window, GLFW_KEY_3) == GLFW_PRESS;
    inputState.toggleConeLights = coneLightsTogglePressed && !m_previousConeLightsTogglePressed;
    m_previousConeLightsTogglePressed = coneLightsTogglePressed;

    const bool moonEmissiveTogglePressed = glfwGetKey(m_window, GLFW_KEY_7) == GLFW_PRESS;
    inputState.toggleMoonEmissive =
        moonEmissiveTogglePressed && !m_previousMoonEmissiveTogglePressed;
    m_previousMoonEmissiveTogglePressed = moonEmissiveTogglePressed;

    const bool sphereEmissiveTogglePressed = glfwGetKey(m_window, GLFW_KEY_8) == GLFW_PRESS;
    inputState.toggleSphereEmissive =
        sphereEmissiveTogglePressed && !m_previousSphereEmissiveTogglePressed;
    m_previousSphereEmissiveTogglePressed = sphereEmissiveTogglePressed;

    const bool coneEmissiveTogglePressed = glfwGetKey(m_window, GLFW_KEY_9) == GLFW_PRESS;
    inputState.toggleConeEmissive =
        coneEmissiveTogglePressed && !m_previousConeEmissiveTogglePressed;
    m_previousConeEmissiveTogglePressed = coneEmissiveTogglePressed;

    const bool moonBackwardPressed = glfwGetKey(m_window, GLFW_KEY_4) == GLFW_PRESS;
    inputState.stepMoonBackward = moonBackwardPressed && !m_previousMoonBackwardPressed;
    m_previousMoonBackwardPressed = moonBackwardPressed;

    const bool moonForwardPressed = glfwGetKey(m_window, GLFW_KEY_5) == GLFW_PRESS;
    inputState.stepMoonForward = moonForwardPressed && !m_previousMoonForwardPressed;
    m_previousMoonForwardPressed = moonForwardPressed;

    const bool moonMotionTogglePressed = glfwGetKey(m_window, GLFW_KEY_6) == GLFW_PRESS;
    inputState.toggleMoonMotion = moonMotionTogglePressed && !m_previousMoonMotionTogglePressed;
    m_previousMoonMotionTogglePressed = moonMotionTogglePressed;

    const bool debugUiTogglePressed = glfwGetKey(m_window, GLFW_KEY_F1) == GLFW_PRESS;
    inputState.toggleDebugUi = debugUiTogglePressed && !m_previousDebugUiTogglePressed;
    m_previousDebugUiTogglePressed = debugUiTogglePressed;

    const bool escapePressed = glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    inputState.toggleCursorCapture = escapePressed && !m_previousEscapePressed;
    m_previousEscapePressed = escapePressed;
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
    const std::filesystem::path runtimeShaderDirectory =
        std::filesystem::current_path() / "shaders";
    if (hasRequiredShaders(runtimeShaderDirectory))
    {
        return runtimeShaderDirectory;
    }

#ifdef ENGINE_SOURCE_DIR
    const std::filesystem::path sourceShaderDirectory =
        std::filesystem::path(ENGINE_SOURCE_DIR) / "shaders";
    if (hasRequiredShaders(sourceShaderDirectory))
    {
        return sourceShaderDirectory;
    }
#endif

    throw std::runtime_error("Unable to locate shader files. Expected a shaders directory next to "
                             "the executable or in the source tree.");
}
} // namespace engine
