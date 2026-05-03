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
    return std::filesystem::exists(directory / "vertex.glsl")
        && std::filesystem::exists(directory / "fragment.glsl");
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
        stream
            << "Creating window "
            << m_windowWidth
            << 'x'
            << m_windowHeight
            << " with OpenGL 3.3 core profile.";
        Log::info("Application", stream.str());
    }

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "EngineStarter", nullptr, nullptr);
    if (m_window == nullptr)
    {
        throw std::runtime_error("Failed to create the GLFW window.");
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, &Application::framebufferSizeCallback);

    Log::info("Application", "Window created and OpenGL context made current.");
    Log::info("Application", "VSync enabled.");

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
    if (m_window != nullptr && glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        Log::info("Application", "Escape pressed, requesting shutdown.");
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
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
    titleStream
        << "EngineStarter - "
        << m_windowWidth
        << 'x'
        << m_windowHeight
        << " - FPS: "
        << static_cast<int>(framesPerSecond + 0.5f);

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

std::filesystem::path Application::resolveShaderDirectory()
{
    const std::filesystem::path runtimeShaderDirectory = std::filesystem::current_path() / "shaders";
    if (hasRequiredShaders(runtimeShaderDirectory))
    {
        return runtimeShaderDirectory;
    }

#ifdef ENGINE_SOURCE_DIR
    const std::filesystem::path sourceShaderDirectory = std::filesystem::path(ENGINE_SOURCE_DIR) / "shaders";
    if (hasRequiredShaders(sourceShaderDirectory))
    {
        return sourceShaderDirectory;
    }
#endif

    throw std::runtime_error(
        "Unable to locate shader files. Expected a shaders directory next to the executable or in the source tree.");
}
} // namespace engine
