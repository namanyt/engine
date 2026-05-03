#pragma once

#include <filesystem>
#include <memory>

struct GLFWwindow;

namespace engine
{
class Renderer;

class Application final
{
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    void run();

private:
    void initializeWindow();
    void initializeRenderer();
    void processInput();
    void shutdown() noexcept;

    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static std::filesystem::path resolveShaderDirectory();

    GLFWwindow* m_window = nullptr;
    std::unique_ptr<Renderer> m_renderer;
    int m_windowWidth = 800;
    int m_windowHeight = 600;
    bool m_glfwInitialized = false;
};
} // namespace engine
