#pragma once

#include <filesystem>

struct GLFWwindow;

namespace engine
{
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

    int framebufferWidth() const noexcept;
    int framebufferHeight() const noexcept;
    const std::filesystem::path& shaderDirectory() const noexcept;

  private:
    void initializeWindow();
    void shutdown() noexcept;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static std::filesystem::path resolveShaderDirectory();

    GLFWwindow* m_window = nullptr;
    std::filesystem::path m_shaderDirectory;
    int m_windowWidth = 1920;
    int m_windowHeight = 1080;
    int m_framebufferWidth = 1920;
    int m_framebufferHeight = 1080;
    bool m_glfwInitialized = false;
};
} // namespace engine
