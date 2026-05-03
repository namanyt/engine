#include "Application.h"

#include "Renderer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <iostream>
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
    try
    {
        initializeWindow();
        initializeRenderer();
    }
    catch (...)
    {
        shutdown();
        throw;
    }
}

Application::~Application()
{
    shutdown();
}

void Application::run()
{
    while (m_window != nullptr && !glfwWindowShouldClose(m_window))
    {
        processInput();

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);

        m_renderer->render(static_cast<float>(glfwGetTime()), framebufferWidth, framebufferHeight);

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void Application::initializeWindow()
{
    if (glfwInit() == GLFW_FALSE)
    {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    m_glfwInitialized = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "EngineStarter", nullptr, nullptr);
    if (m_window == nullptr)
    {
        throw std::runtime_error("Failed to create the GLFW window.");
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, &Application::framebufferSizeCallback);

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0)
    {
        throw std::runtime_error("Failed to load OpenGL functions with GLAD.");
    }

    glViewport(0, 0, m_windowWidth, m_windowHeight);

    const auto* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const auto* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const auto* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    std::cout << "OpenGL vendor   : " << (vendor != nullptr ? vendor : "unknown") << '\n';
    std::cout << "OpenGL renderer : " << (renderer != nullptr ? renderer : "unknown") << '\n';
    std::cout << "OpenGL version  : " << (version != nullptr ? version : "unknown") << '\n';
}

void Application::initializeRenderer()
{
    m_renderer = std::make_unique<Renderer>(resolveShaderDirectory());
}

void Application::processInput()
{
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

void Application::shutdown() noexcept
{
    m_renderer.reset();

    if (m_window != nullptr)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    if (m_glfwInitialized)
    {
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
        application->m_windowWidth = width;
        application->m_windowHeight = height;
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
