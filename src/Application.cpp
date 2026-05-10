#include "Application.h"

#include "assets/AssetManager.h"
#include "core/AudioSystem.h"
#include "core/Log.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace engine
{
namespace
{
constexpr int kWindowedMinWidth = 1280;
constexpr int kWindowedMinHeight = 720;
constexpr int kWindowedMaxWidth = 1920;
constexpr int kWindowedMaxHeight = 1080;

float& audioSettingsVolume(Application::AudioSettings& settings, const AudioCategory category)
{
    switch (category)
    {
    case AudioCategory::Music:
        return settings.musicVolume;
    case AudioCategory::UI:
        return settings.uiVolume;
    case AudioCategory::VN:
        return settings.vnVolume;
    case AudioCategory::Ambient:
        return settings.ambientVolume;
    case AudioCategory::SFX:
    default:
        return settings.sfxVolume;
    }
}

float audioSettingsVolume(const Application::AudioSettings& settings, const AudioCategory category)
{
    switch (category)
    {
    case AudioCategory::Music:
        return settings.musicVolume;
    case AudioCategory::UI:
        return settings.uiVolume;
    case AudioCategory::VN:
        return settings.vnVolume;
    case AudioCategory::Ambient:
        return settings.ambientVolume;
    case AudioCategory::SFX:
    default:
        return settings.sfxVolume;
    }
}

const char* windowModeToString(const Application::WindowMode mode)
{
    switch (mode)
    {
    case Application::WindowMode::Windowed:
        return "windowed";
    case Application::WindowMode::BorderlessFullscreen:
        return "borderless";
    case Application::WindowMode::ExclusiveFullscreen:
        return "exclusive";
    default:
        return "windowed";
    }
}

Application::WindowMode parseWindowModeSetting(const std::string& value)
{
    if (value == "borderless" || value == "windowed_fullscreen")
    {
        return Application::WindowMode::BorderlessFullscreen;
    }

    if (value == "exclusive" || value == "exclusive_fullscreen")
    {
        return Application::WindowMode::ExclusiveFullscreen;
    }

    return Application::WindowMode::Windowed;
}

Application::DisplaySettings sanitizeDisplaySettings(Application::DisplaySettings settings)
{
    constexpr int kMinimumWidth = 640;
    constexpr int kMinimumHeight = 360;
    settings.width = std::max(settings.width, kMinimumWidth);
    settings.height = std::max(settings.height, kMinimumHeight);
    return settings;
}

float clampUnitFloat(const float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float clampTypingCharactersPerSecond(const float value)
{
    return std::clamp(value, 12.0f, 48.0f);
}

float clampMouseSensitivity(const float value)
{
    return std::clamp(value, 0.01f, 1.0f);
}

struct LoadedEngineSettings final
{
    Application::DisplaySettings display{};
    Application::AudioSettings audio{};
    Application::VnSettings vn{};
    Application::InputSettings input{};
};

bool parseBoolSetting(const std::string& value)
{
    return value == "1" || value == "true" || value == "on" || value == "yes";
}

LoadedEngineSettings loadEngineSettings(const std::filesystem::path& settingsPath)
{
    LoadedEngineSettings settings{};
    std::ifstream stream(settingsPath);
    if (!stream.is_open())
    {
        return settings;
    }

    std::string line;
    while (std::getline(stream, line))
    {
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos)
        {
            continue;
        }

        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        if (key == "width")
        {
            settings.display.width = std::max(1, std::stoi(value));
        }
        else if (key == "height")
        {
            settings.display.height = std::max(1, std::stoi(value));
        }
        else if (key == "fullscreen")
        {
            settings.display.windowMode = parseBoolSetting(value)
                                              ? Application::WindowMode::ExclusiveFullscreen
                                              : Application::WindowMode::Windowed;
        }
        else if (key == "window_mode")
        {
            settings.display.windowMode = parseWindowModeSetting(value);
        }
        else if (key == "vsync")
        {
            settings.display.vSyncEnabled = parseBoolSetting(value);
        }
        else if (key == "master_volume")
        {
            settings.audio.masterVolume = clampUnitFloat(std::stof(value));
        }
        else if (key == "music_volume")
        {
            settings.audio.musicVolume = clampUnitFloat(std::stof(value));
        }
        else if (key == "ui_volume")
        {
            settings.audio.uiVolume = clampUnitFloat(std::stof(value));
        }
        else if (key == "vn_volume")
        {
            settings.audio.vnVolume = clampUnitFloat(std::stof(value));
        }
        else if (key == "ambient_volume")
        {
            settings.audio.ambientVolume = clampUnitFloat(std::stof(value));
        }
        else if (key == "sfx_volume")
        {
            settings.audio.sfxVolume = clampUnitFloat(std::stof(value));
        }
        else if (key == "vn_typing_cps")
        {
            settings.vn.typingCharactersPerSecond =
                clampTypingCharactersPerSecond(std::stof(value));
        }
        else if (key == "vn_auto_advance")
        {
            settings.vn.autoAdvanceEnabled = parseBoolSetting(value);
        }
        else if (key == "mouse_sensitivity")
        {
            settings.input.mouseSensitivity = clampMouseSensitivity(std::stof(value));
        }
    }

    settings.display = sanitizeDisplaySettings(settings.display);
    return settings;
}
} // namespace

Application::Application()
{
    Log::info("Application", "Starting application bootstrap.");

    try
    {
        m_settingsFilePath = resolveSettingsFilePath();
        const LoadedEngineSettings loadedSettings = loadEngineSettings(m_settingsFilePath);
        m_displaySettings = loadedSettings.display;
        m_audioSettings = loadedSettings.audio;
        m_vnSettings = loadedSettings.vn;
        m_inputSettings = loadedSettings.input;
        m_windowWidth = m_displaySettings.width;
        m_windowHeight = m_displaySettings.height;
        m_windowedWidth = m_displaySettings.width;
        m_windowedHeight = m_displaySettings.height;
        initializeWindow();
        m_assetRootDirectory = resolveAssetRootDirectory();
        m_shaderDirectory = resolveShaderDirectory();
        m_assetManager = std::make_shared<AssetManager>();
        const std::size_t discoveredAssets = m_assetManager->discover(m_assetRootDirectory);
        m_audioSystem = std::make_unique<AudioSystem>();
        m_audioSystem->initialize();
        m_audioSystem->setMasterVolume(m_audioSettings.masterVolume);
        for (const AudioCategory category : kAllAudioCategories)
        {
            m_audioSystem->setCategoryVolume(category,
                                             audioSettingsVolume(m_audioSettings, category));
        }

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
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, &Application::framebufferSizeCallback);
    glfwSetWindowSizeCallback(m_window, &Application::windowSizeCallback);
    glfwSetCursorPosCallback(m_window, &Application::cursorPositionCallback);
    glfwSetScrollCallback(m_window, &Application::scrollCallback);
    glfwGetWindowPos(m_window, &m_windowedPosX, &m_windowedPosY);
    applyWindowResizeConstraints();

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

    setVSyncEnabled(m_displaySettings.vSyncEnabled);
    if (m_displaySettings.windowMode != WindowMode::Windowed)
    {
        setWindowMode(m_displaySettings.windowMode);
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
        toggleBorderlessFullscreen();
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
    m_pendingMouseScrollDelta = 0.0f;
}

RawInputState Application::consumeRawInputState()
{
    RawInputState inputState{};
    inputState.cursorCaptured = m_cursorCaptured;
    inputState.mouseDelta = m_pendingMouseDelta;
    inputState.mouseScrollDelta = m_pendingMouseScrollDelta;
    inputState.windowSize =
        Vec2{static_cast<float>(m_windowWidth), static_cast<float>(m_windowHeight)};
    if (m_window != nullptr)
    {
        double cursorX = 0.0;
        double cursorY = 0.0;
        glfwGetCursorPos(m_window, &cursorX, &cursorY);
        inputState.mousePosition = Vec2{static_cast<float>(cursorX), static_cast<float>(cursorY)};
    }
    updateKeyboardState(inputState);
    m_pendingMouseDelta = Vec2{};
    m_pendingMouseScrollDelta = 0.0f;
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

void Application::setWindowResolution(int width, int height)
{
    const DisplaySettings sanitized = sanitizeDisplaySettings(DisplaySettings{
        width, height, m_displaySettings.windowMode, m_displaySettings.vSyncEnabled});

    m_displaySettings.width = sanitized.width;
    m_displaySettings.height = sanitized.height;
    m_windowedWidth = sanitized.width;
    m_windowedHeight = sanitized.height;

    if (m_window == nullptr)
    {
        persistSettings();
        return;
    }

    if (m_windowMode == WindowMode::ExclusiveFullscreen)
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (monitor != nullptr)
        {
            glfwSetWindowMonitor(m_window, monitor, 0, 0, sanitized.width, sanitized.height,
                                 GLFW_DONT_CARE);
        }
    }
    else if (m_windowMode == WindowMode::Windowed)
    {
        applyWindowResizeConstraints();
        glfwSetWindowSize(m_window, sanitized.width, sanitized.height);
        m_windowWidth = sanitized.width;
        m_windowHeight = sanitized.height;
    }
    persistSettings();

    std::ostringstream stream;
    stream << (m_windowMode == WindowMode::BorderlessFullscreen ? "Stored" : "Applied")
           << " window resolution " << sanitized.width << 'x' << sanitized.height << '.';
    Log::info("Application", stream.str());
}

void Application::setWindowMode(WindowMode mode)
{
    if (m_windowMode == mode)
    {
        return;
    }

    if (m_window == nullptr)
    {
        m_windowMode = mode;
        m_displaySettings.windowMode = mode;
        persistSettings();
        return;
    }

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = monitor != nullptr ? glfwGetVideoMode(monitor) : nullptr;
    if (mode != WindowMode::Windowed && (monitor == nullptr || videoMode == nullptr))
    {
        Log::warning("Application", "Unable to change fullscreen mode: no monitor available.");
        return;
    }

    if (m_windowMode == WindowMode::Windowed)
    {
        glfwGetWindowPos(m_window, &m_windowedPosX, &m_windowedPosY);
        glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);
    }

    if (mode == WindowMode::Windowed)
    {
        glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(m_window, nullptr, m_windowedPosX, m_windowedPosY,
                             m_displaySettings.width, m_displaySettings.height, GLFW_DONT_CARE);
        m_windowWidth = m_displaySettings.width;
        m_windowHeight = m_displaySettings.height;
        applyWindowResizeConstraints();
        Log::info("Application", "Returned to windowed mode.");
    }
    else if (mode == WindowMode::BorderlessFullscreen)
    {
        int monitorX = 0;
        int monitorY = 0;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(m_window, nullptr, monitorX, monitorY, videoMode->width,
                             videoMode->height, GLFW_DONT_CARE);
        m_windowWidth = videoMode->width;
        m_windowHeight = videoMode->height;
        applyWindowResizeConstraints();
        Log::info("Application", "Entered borderless fullscreen mode.");
    }
    else
    {
        glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(m_window, monitor, 0, 0, m_displaySettings.width,
                             m_displaySettings.height, GLFW_DONT_CARE);
        m_windowWidth = m_displaySettings.width;
        m_windowHeight = m_displaySettings.height;
        applyWindowResizeConstraints();

        std::ostringstream stream;
        stream << "Entered exclusive fullscreen at " << m_displaySettings.width << 'x'
               << m_displaySettings.height << '.';
        Log::info("Application", stream.str());
    }

    m_windowMode = mode;
    m_displaySettings.windowMode = mode;
    persistSettings();
}

void Application::setExclusiveFullscreen(bool enabled)
{
    setWindowMode(enabled ? WindowMode::ExclusiveFullscreen : WindowMode::Windowed);
}

void Application::setVSyncEnabled(bool enabled)
{
    m_displaySettings.vSyncEnabled = enabled;
    if (m_window != nullptr)
    {
        glfwSwapInterval(enabled ? 1 : 0);
    }

    persistSettings();
    Log::info("Application", enabled ? "VSync enabled." : "VSync disabled.");
}

void Application::setMasterVolume(float volume)
{
    m_audioSettings.masterVolume = clampUnitFloat(volume);
    if (m_audioSystem != nullptr)
    {
        m_audioSystem->setMasterVolume(m_audioSettings.masterVolume);
    }
    persistSettings();
}

void Application::setAudioCategoryVolume(const AudioCategory category, float volume)
{
    audioSettingsVolume(m_audioSettings, category) = clampUnitFloat(volume);
    if (m_audioSystem != nullptr)
    {
        m_audioSystem->setCategoryVolume(category, audioSettingsVolume(m_audioSettings, category));
    }
    persistSettings();
}

void Application::setMusicVolume(float volume)
{
    setAudioCategoryVolume(AudioCategory::Music, volume);
}

void Application::setVnTypingCharactersPerSecond(float charactersPerSecond)
{
    m_vnSettings.typingCharactersPerSecond = clampTypingCharactersPerSecond(charactersPerSecond);
    persistSettings();
}

void Application::setVnAutoAdvanceEnabled(bool enabled)
{
    m_vnSettings.autoAdvanceEnabled = enabled;
    persistSettings();
}

void Application::setMouseSensitivity(float sensitivity)
{
    m_inputSettings.mouseSensitivity = clampMouseSensitivity(sensitivity);
    persistSettings();
}

void Application::applyWindowResizeConstraints()
{
    if (m_window == nullptr)
    {
        return;
    }

    if (m_windowMode == WindowMode::Windowed)
    {
        glfwSetWindowAspectRatio(m_window, 16, 9);
        glfwSetWindowSizeLimits(m_window, kWindowedMinWidth, kWindowedMinHeight, kWindowedMaxWidth,
                                kWindowedMaxHeight);
        return;
    }

    glfwSetWindowAspectRatio(m_window, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetWindowSizeLimits(m_window, GLFW_DONT_CARE, GLFW_DONT_CARE, GLFW_DONT_CARE,
                            GLFW_DONT_CARE);
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

AudioSystem& Application::audioSystem() noexcept
{
    return *m_audioSystem;
}

const AudioSystem& Application::audioSystem() const noexcept
{
    return *m_audioSystem;
}

bool Application::isCursorCaptured() const noexcept
{
    return m_cursorCaptured;
}

bool Application::isBorderlessFullscreen() const noexcept
{
    return m_windowMode == WindowMode::BorderlessFullscreen;
}

bool Application::isExclusiveFullscreen() const noexcept
{
    return m_windowMode == WindowMode::ExclusiveFullscreen;
}

bool Application::isVSyncEnabled() const noexcept
{
    return m_displaySettings.vSyncEnabled;
}

Application::WindowMode Application::windowMode() const noexcept
{
    return m_windowMode;
}

const Application::DisplaySettings& Application::displaySettings() const noexcept
{
    return m_displaySettings;
}

const Application::AudioSettings& Application::audioSettings() const noexcept
{
    return m_audioSettings;
}

float Application::audioCategoryVolume(const AudioCategory category) const noexcept
{
    return audioSettingsVolume(m_audioSettings, category);
}

const Application::VnSettings& Application::vnSettings() const noexcept
{
    return m_vnSettings;
}

const Application::InputSettings& Application::inputSettings() const noexcept
{
    return m_inputSettings;
}

GLFWwindow* Application::nativeWindow() const noexcept
{
    return m_window;
}

void Application::toggleBorderlessFullscreen()
{
    setWindowMode(m_windowMode == WindowMode::Windowed ? WindowMode::BorderlessFullscreen
                                                       : WindowMode::Windowed);
}

void Application::persistSettings() const
{
    if (m_settingsFilePath.empty())
    {
        return;
    }

    std::ofstream stream(m_settingsFilePath, std::ios::trunc);
    if (!stream.is_open())
    {
        Log::warning("Application",
                     "Unable to persist settings to " + m_settingsFilePath.string() + ".");
        return;
    }

    stream << "width=" << m_displaySettings.width << '\n';
    stream << "height=" << m_displaySettings.height << '\n';
    stream << "window_mode=" << windowModeToString(m_displaySettings.windowMode) << '\n';
    stream << "fullscreen="
           << (m_displaySettings.windowMode == WindowMode::ExclusiveFullscreen ? 1 : 0) << '\n';
    stream << "vsync=" << (m_displaySettings.vSyncEnabled ? 1 : 0) << '\n';
    stream << "master_volume=" << m_audioSettings.masterVolume << '\n';
    stream << "music_volume=" << m_audioSettings.musicVolume << '\n';
    stream << "ui_volume=" << m_audioSettings.uiVolume << '\n';
    stream << "vn_volume=" << m_audioSettings.vnVolume << '\n';
    stream << "ambient_volume=" << m_audioSettings.ambientVolume << '\n';
    stream << "sfx_volume=" << m_audioSettings.sfxVolume << '\n';
    stream << "vn_typing_cps=" << m_vnSettings.typingCharactersPerSecond << '\n';
    stream << "vn_auto_advance=" << (m_vnSettings.autoAdvanceEnabled ? 1 : 0) << '\n';
    stream << "mouse_sensitivity=" << m_inputSettings.mouseSensitivity << '\n';
}

void Application::shutdown() noexcept
{
    if (m_audioSystem != nullptr)
    {
        Log::info("Application", "Shutting down audio system.");
        m_audioSystem->shutdown();
        m_audioSystem.reset();
    }

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
    captureButton(inputState.keyE, glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS,
                  m_previousEPressed);
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
    captureButton(inputState.mouseLeft,
                  glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS,
                  m_previousMouseLeftPressed);
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

    if (application->m_windowMode == WindowMode::Windowed)
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

void Application::scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    static_cast<void>(xOffset);

    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (application == nullptr)
    {
        return;
    }

    application->m_pendingMouseScrollDelta += static_cast<float>(yOffset);
}

std::filesystem::path Application::resolveShaderDirectory()
{
    return resolveAssetRootDirectory() / "shaders";
}

std::filesystem::path Application::resolveSettingsFilePath()
{
#ifdef ENGINE_SOURCE_DIR
    return std::filesystem::path(ENGINE_SOURCE_DIR) / "engine_settings.ini";
#else
    return std::filesystem::current_path() / "engine_settings.ini";
#endif
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
