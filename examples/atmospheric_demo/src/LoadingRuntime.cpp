#include "LoadingRuntime.h"

#include "Application.h"
#include "core/AudioSystem.h"
#include "core/Log.h"
#include "core/RenderPipeline.h"
#include "core/Renderer.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace engine
{
namespace
{
constexpr float kDeferredLoadStartDelaySeconds = 0.18f;
constexpr float kDisclaimerFadeDurationSeconds = 1.2f;
constexpr float kBootSequenceDisclaimerDurationSeconds = 4.0f;
constexpr float kBootSequenceFadeOutDurationSeconds = 0.5f;
constexpr float kPersistentMusicFadeOutDurationSeconds = 1.5f;
constexpr float kBootSequenceDefaultLineAdvanceSeconds = 0.065f;
constexpr float kBootSequenceBurstLineAdvanceSeconds = 0.028f;
constexpr float kBootSequenceCursorBlinkSeconds = 0.24f;
constexpr std::string_view kBootSequenceAssetPath = "boot/boot_sequence.txt";
constexpr std::string_view kMainMenuMusicPlaybackId{"main-menu-music"};

struct ParsedBootSequenceColor final
{
    Color color = Color::white();
    bool hasColor = false;
};

std::string trimAscii(std::string value)
{
    auto isNotSpace = [](unsigned char character) { return !std::isspace(character); };

    value.erase(value.begin(), std::find_if(value.begin(), value.end(), isNotSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
    return value;
}

std::string uppercaseAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
                   { return static_cast<char>(std::toupper(character)); });
    return value;
}

int parseMilliseconds(std::istringstream& stream, int defaultValue)
{
    int value = defaultValue;
    if (!(stream >> value))
    {
        return defaultValue;
    }

    return std::max(value, 0);
}

ParsedBootSequenceColor parseBootSequenceColor(std::string token)
{
    token = uppercaseAscii(trimAscii(std::move(token)));
    if (token.empty() || token == "NONE" || token == "DEFAULT")
    {
        return {};
    }

    if (token == "ERROR" || token == "RED")
    {
        return ParsedBootSequenceColor{Color{0.92f, 0.36f, 0.36f, 1.0f}, true};
    }

    if (token == "WARNING" || token == "WARN" || token == "YELLOW" || token == "AMBER")
    {
        return ParsedBootSequenceColor{Color{0.90f, 0.74f, 0.35f, 1.0f}, true};
    }

    if (token == "SUCCESS" || token == "OK" || token == "GREEN")
    {
        return ParsedBootSequenceColor{Color{0.48f, 0.82f, 0.55f, 1.0f}, true};
    }

    if (token == "INFO" || token == "BLUE")
    {
        return ParsedBootSequenceColor{Color{0.45f, 0.67f, 0.92f, 1.0f}, true};
    }

    if (token == "NOTICE" || token == "CYAN")
    {
        return ParsedBootSequenceColor{Color{0.45f, 0.84f, 0.84f, 1.0f}, true};
    }

    if (token == "MAGENTA" || token == "PURPLE")
    {
        return ParsedBootSequenceColor{Color{0.79f, 0.58f, 0.88f, 1.0f}, true};
    }

    if (token == "DIM" || token == "GRAY" || token == "GREY")
    {
        return ParsedBootSequenceColor{Color{0.54f, 0.54f, 0.54f, 1.0f}, true};
    }

    if (token == "WHITE")
    {
        return ParsedBootSequenceColor{Color{0.92f, 0.92f, 0.92f, 1.0f}, true};
    }

    return {};
}

bool bootSequenceTextEntriesEqual(const BootSequenceTextEntry& left,
                                  const BootSequenceTextEntry& right)
{
    return left.text == right.text && left.hasColor == right.hasColor &&
           (!left.hasColor || (left.color.r == right.color.r && left.color.g == right.color.g &&
                               left.color.b == right.color.b && left.color.a == right.color.a));
}
} // namespace

LoadingRuntime::LoadingRuntime(std::unique_ptr<RuntimeMode> nextRuntimeMode,
                               float minimumDurationSeconds, std::string nextRuntimeLabel,
                               LoadingScreenStyle loadingScreenStyle)
    : m_nextRuntimeMode(std::move(nextRuntimeMode)),
      m_nextRuntimeLabel(std::move(nextRuntimeLabel)), m_loadingScreenStyle(loadingScreenStyle),
      m_minimumDurationSeconds(minimumDurationSeconds)
{
    if (m_nextRuntimeLabel.empty() && m_nextRuntimeMode != nullptr)
    {
        m_nextRuntimeLabel = m_nextRuntimeMode->name();
    }
}

LoadingRuntime::~LoadingRuntime() = default;

const char* LoadingRuntime::name() const
{
    return "LoadingRuntime";
}

void LoadingRuntime::activate(ActivationContext& activationContext)
{
    m_assetManager = activationContext.assetManager;
    m_shaderLibrary = &activationContext.shaderLibrary;
    m_assetRootDirectory = activationContext.assetRootDirectory;
    m_shaderDirectory = activationContext.shaderDirectory;
    activationContext.renderer.prepareOverlayRenderingResources();
    activationContext.application.setCursorCaptured(false);
    m_elapsedSeconds = 0.0f;
    m_loadingCompletedAtSeconds = -1.0f;
    m_bootSequenceScriptDurationSeconds = 0.0f;
    m_bootSequenceVisibleLineRevision = -1;
    m_bootSequenceCursorRevision = false;
    m_persistentMusicFadeRequested = false;
    m_bootSequenceStatusRevision = BootSequenceTextEntry{};
    m_loadingStarted = false;
    m_transitionQueued = false;
    m_bootSequenceEvents.clear();
    if (m_loadingScreenStyle == LoadingScreenStyle::DisclaimerBootSequence)
    {
        loadBootSequenceScript();
    }

    refreshOverlay();
    setProgress(activationContext.application, 0.05f,
                m_loadingScreenStyle == LoadingScreenStyle::Disclaimer ? "entering disclaimer"
                                                                       : "starting load");

    Log::info("LoadingRuntime", "Activated loading runtime.");
}

void LoadingRuntime::deactivate(Renderer& renderer)
{
    m_loadingOverlay.reset();
    m_assetManager.reset();
    m_shaderLibrary = nullptr;
    m_assetRootDirectory.clear();
    m_shaderDirectory.clear();
    m_progressPhase.clear();
    m_activationProgress = 0.0f;
    m_bootSequenceScriptDurationSeconds = 0.0f;
    m_bootSequenceVisibleLineRevision = -1;
    m_bootSequenceCursorRevision = false;
    m_persistentMusicFadeRequested = false;
    m_bootSequenceStatusRevision = BootSequenceTextEntry{};
    m_transitionQueued = false;
    m_bootSequenceEvents.clear();
    renderer.clearRuntimeOverlayTexture();
}

void LoadingRuntime::update(const UpdateContext& updateContext)
{
    (void)updateContext.inputState;
    m_elapsedSeconds += updateContext.deltaSeconds;

    if (!m_loadingStarted && m_nextRuntimeMode != nullptr &&
        m_elapsedSeconds >= kDeferredLoadStartDelaySeconds)
    {
        beginDeferredLoad(updateContext);
    }

    refreshBootSequenceOverlayIfNeeded();

    if (m_loadingScreenStyle == LoadingScreenStyle::DisclaimerBootSequence &&
        !m_persistentMusicFadeRequested)
    {
        const float fadeStartTimeSeconds =
            std::max(transitionReadyTimeSeconds() - kPersistentMusicFadeOutDurationSeconds, 0.0f);
        if (m_elapsedSeconds >= fadeStartTimeSeconds)
        {
            updateContext.application.audioSystem().fadeOutPersistent(
                kMainMenuMusicPlaybackId, kPersistentMusicFadeOutDurationSeconds);
            m_persistentMusicFadeRequested = true;
        }
    }

    updateProgressTitle(updateContext.application);
    if (m_nextRuntimeMode == nullptr || m_loadingCompletedAtSeconds < 0.0f ||
        m_elapsedSeconds < transitionReadyTimeSeconds())
    {
        return;
    }

    setProgress(updateContext.application, 1.0f, "complete");
    Log::info("LoadingRuntime", "Loading complete. Transitioning to " + m_nextRuntimeLabel + ".");
    m_transitionQueued = true;
    requestTransition(std::move(m_nextRuntimeMode));
}

void LoadingRuntime::render(const RenderContext& renderContext)
{
    applyOverlayTexture(renderContext.renderer);

    if (shouldRenderPreparedPreview() && m_nextRuntimeMode != nullptr &&
        m_nextRuntimeMode->canRenderLoadingPreview())
    {
        m_nextRuntimeMode->renderLoadingPreview(renderContext);
        return;
    }

    renderContext.renderer.setViewport(renderContext.framebufferWidth,
                                       renderContext.framebufferHeight);
    renderContext.renderPipeline.renderOverlayFrame(activeClearColor());
}

#if defined(ENGINE_ENABLE_DEBUG_UI) && !defined(NDEBUG)
void LoadingRuntime::drawDebugUi(const DebugUiContext& debugUiContext)
{
    (void)debugUiContext;
}
#endif

Color LoadingRuntime::activeClearColor() const
{
    return Color{0.0f, 0.0f, 0.0f, 1.0f};
}

void LoadingRuntime::applyOverlayTexture(Renderer& renderer) const
{
    if (!m_loadingOverlay.valid())
    {
        renderer.clearRuntimeOverlayTexture();
        return;
    }

    m_loadingOverlay.apply(
        renderer, RuntimeOverlayOptions{RuntimeOverlayLayout::FullScreen, overlayOpacity()});
}

void LoadingRuntime::beginDeferredLoad(const UpdateContext& updateContext)
{
    m_loadingStarted = true;
    setProgress(updateContext.application, 0.18f, "priming renderer");

    RuntimeMode::ActivationContext activationContext{
        updateContext.application,
        updateContext.renderer,
        updateContext.renderPipeline,
        m_assetManager,
        *m_shaderLibrary,
        m_assetRootDirectory,
        m_shaderDirectory,
    };

    setProgress(updateContext.application, 0.35f, "compiling shaders");
    m_nextRuntimeMode->prepareActivation(activationContext);
    m_loadingCompletedAtSeconds = m_elapsedSeconds;
    setProgress(updateContext.application, 0.88f, "scene ready");
}

void LoadingRuntime::loadBootSequenceScript()
{
    m_bootSequenceEvents.clear();
    m_bootSequenceScriptDurationSeconds = 0.0f;

    const std::filesystem::path scriptPath = m_assetRootDirectory / kBootSequenceAssetPath;
    std::ifstream input(scriptPath);
    if (!input)
    {
        throw std::runtime_error("Failed to open boot sequence asset: " + scriptPath.string());
    }

    float currentTimeSeconds = 0.0f;
    float normalLineAdvanceSeconds = kBootSequenceDefaultLineAdvanceSeconds;
    bool burstMode = false;
    ParsedBootSequenceColor currentLineColor{};
    ParsedBootSequenceColor currentStatusColor{};
    std::string rawLine;
    while (std::getline(input, rawLine))
    {
        const std::string trimmed = trimAscii(rawLine);
        if (trimmed.empty())
        {
            continue;
        }

        if (!trimmed.empty() && trimmed.front() == '#')
        {
            std::istringstream directiveStream(trimmed.substr(1));
            std::string command;
            directiveStream >> command;
            command = uppercaseAscii(command);

            if (command == "PAUSE" || command == "WAIT")
            {
                currentTimeSeconds += parseMilliseconds(directiveStream, 180) / 1000.0f;
                burstMode = false;
                continue;
            }

            if (command == "CURSOR")
            {
                BootSequenceEvent event{};
                event.type = BootSequenceEvent::Type::Cursor;
                event.timeSeconds = currentTimeSeconds;
                event.durationSeconds = parseMilliseconds(directiveStream, 280) / 1000.0f;
                m_bootSequenceEvents.push_back(std::move(event));
                currentTimeSeconds += m_bootSequenceEvents.back().durationSeconds;
                burstMode = false;
                continue;
            }

            if (command == "STATUS")
            {
                BootSequenceEvent event{};
                event.type = BootSequenceEvent::Type::Status;
                event.timeSeconds = currentTimeSeconds;
                std::string statusText;
                std::getline(directiveStream, statusText);
                event.text = trimAscii(statusText);
                event.color = currentStatusColor.color;
                event.hasColor = currentStatusColor.hasColor;
                m_bootSequenceEvents.push_back(std::move(event));
                burstMode = false;
                continue;
            }

            if (command == "COLOR")
            {
                std::string colorToken;
                directiveStream >> colorToken;
                currentLineColor = parseBootSequenceColor(colorToken);
                burstMode = false;
                continue;
            }

            if (command == "STATUS_COLOR")
            {
                std::string colorToken;
                directiveStream >> colorToken;
                currentStatusColor = parseBootSequenceColor(colorToken);
                burstMode = false;
                continue;
            }

            if (command == "BURST")
            {
                burstMode = true;
                continue;
            }

            if (command == "PACE" || command == "RATE")
            {
                normalLineAdvanceSeconds = parseMilliseconds(directiveStream, 65) / 1000.0f;
                burstMode = false;
                continue;
            }

            if (command == "NORMAL")
            {
                burstMode = false;
                continue;
            }

            continue;
        }

        BootSequenceEvent event{};
        event.type = BootSequenceEvent::Type::Line;
        event.timeSeconds = currentTimeSeconds;
        event.text = rawLine;
        event.color = currentLineColor.color;
        event.hasColor = currentLineColor.hasColor;
        m_bootSequenceEvents.push_back(std::move(event));
        currentTimeSeconds +=
            burstMode ? kBootSequenceBurstLineAdvanceSeconds : normalLineAdvanceSeconds;
    }

    m_bootSequenceScriptDurationSeconds = currentTimeSeconds;
}

BootSequenceTextEntry LoadingRuntime::bootSequenceStatusText() const
{
    if (m_loadingScreenStyle != LoadingScreenStyle::DisclaimerBootSequence ||
        m_elapsedSeconds < kBootSequenceDisclaimerDurationSeconds)
    {
        return {};
    }

    const float bootElapsedSeconds = m_elapsedSeconds - kBootSequenceDisclaimerDurationSeconds;
    BootSequenceTextEntry statusText{};
    for (const BootSequenceEvent& event : m_bootSequenceEvents)
    {
        if (event.timeSeconds > bootElapsedSeconds)
        {
            break;
        }

        if (event.type == BootSequenceEvent::Type::Status)
        {
            statusText.text = event.text;
            statusText.color = event.color;
            statusText.hasColor = event.hasColor;
        }
    }

    return statusText;
}

float LoadingRuntime::bootSequencePresentationEndTimeSeconds() const
{
    return std::max(m_minimumDurationSeconds,
                    kBootSequenceDisclaimerDurationSeconds + m_bootSequenceScriptDurationSeconds);
}

int LoadingRuntime::bootSequenceVisibleLineCount() const
{
    if (m_loadingScreenStyle != LoadingScreenStyle::DisclaimerBootSequence ||
        m_elapsedSeconds < kBootSequenceDisclaimerDurationSeconds)
    {
        return 0;
    }

    const float bootElapsedSeconds = m_elapsedSeconds - kBootSequenceDisclaimerDurationSeconds;
    int visibleLineCount = 0;
    for (const BootSequenceEvent& event : m_bootSequenceEvents)
    {
        if (event.timeSeconds > bootElapsedSeconds)
        {
            break;
        }

        if (event.type == BootSequenceEvent::Type::Line)
        {
            ++visibleLineCount;
        }
    }

    return visibleLineCount;
}

bool LoadingRuntime::bootSequenceCursorVisible() const
{
    if (m_loadingScreenStyle != LoadingScreenStyle::DisclaimerBootSequence ||
        m_elapsedSeconds < kBootSequenceDisclaimerDurationSeconds)
    {
        return false;
    }

    const float bootElapsedSeconds = m_elapsedSeconds - kBootSequenceDisclaimerDurationSeconds;
    bool cursorActive = false;
    for (const BootSequenceEvent& event : m_bootSequenceEvents)
    {
        if (event.timeSeconds > bootElapsedSeconds)
        {
            break;
        }

        if (event.type == BootSequenceEvent::Type::Cursor &&
            bootElapsedSeconds < event.timeSeconds + event.durationSeconds)
        {
            cursorActive = true;
        }
    }

    if (!cursorActive && bootElapsedSeconds >= m_bootSequenceScriptDurationSeconds)
    {
        cursorActive = true;
    }

    if (!cursorActive)
    {
        return false;
    }

    const int blinkStep = static_cast<int>(bootElapsedSeconds / kBootSequenceCursorBlinkSeconds);
    return (blinkStep % 2) == 0;
}

std::vector<BootSequenceTextEntry> LoadingRuntime::visibleBootSequenceLines() const
{
    std::vector<BootSequenceTextEntry> lines;
    if (m_loadingScreenStyle != LoadingScreenStyle::DisclaimerBootSequence ||
        m_elapsedSeconds < kBootSequenceDisclaimerDurationSeconds)
    {
        return lines;
    }

    const float bootElapsedSeconds = m_elapsedSeconds - kBootSequenceDisclaimerDurationSeconds;
    for (const BootSequenceEvent& event : m_bootSequenceEvents)
    {
        if (event.timeSeconds > bootElapsedSeconds)
        {
            break;
        }

        if (event.type == BootSequenceEvent::Type::Line)
        {
            lines.push_back(BootSequenceTextEntry{event.text, event.color, event.hasColor});
        }
    }

    return lines;
}

float LoadingRuntime::bootSequenceFadeStartTimeSeconds() const
{
    if (m_loadingCompletedAtSeconds < 0.0f)
    {
        return bootSequencePresentationEndTimeSeconds();
    }

    return std::max(m_loadingCompletedAtSeconds, bootSequencePresentationEndTimeSeconds()) -
           kBootSequenceFadeOutDurationSeconds;
}

float LoadingRuntime::overlayOpacity() const
{
    if (m_loadingScreenStyle == LoadingScreenStyle::DisclaimerBootSequence)
    {
        if (m_elapsedSeconds < kBootSequenceDisclaimerDurationSeconds)
        {
            const float fadeIn =
                std::clamp(m_elapsedSeconds / kDisclaimerFadeDurationSeconds, 0.0f, 1.0f);
            const float fadeOut =
                1.0f - std::clamp((m_elapsedSeconds - (kBootSequenceDisclaimerDurationSeconds -
                                                       kDisclaimerFadeDurationSeconds)) /
                                      kDisclaimerFadeDurationSeconds,
                                  0.0f, 1.0f);
            return std::clamp(std::min(fadeIn, fadeOut), 0.0f, 1.0f);
        }

        if (m_loadingCompletedAtSeconds < 0.0f)
        {
            return 1.0f;
        }

        return 1.0f;
    }

    if (m_loadingScreenStyle != LoadingScreenStyle::Disclaimer)
    {
        return 1.0f;
    }

    const float fadeIn = std::clamp(m_elapsedSeconds / kDisclaimerFadeDurationSeconds, 0.0f, 1.0f);
    if (m_loadingCompletedAtSeconds < 0.0f)
    {
        return fadeIn;
    }

    const float fadeOutStart =
        std::max(m_loadingCompletedAtSeconds,
                 std::max(m_minimumDurationSeconds - kDisclaimerFadeDurationSeconds, 0.0f));
    const float fadeOut =
        1.0f -
        std::clamp((m_elapsedSeconds - fadeOutStart) / kDisclaimerFadeDurationSeconds, 0.0f, 1.0f);
    return std::clamp(std::min(fadeIn, fadeOut), 0.0f, 1.0f);
}

float LoadingRuntime::transitionReadyTimeSeconds() const
{
    if (m_loadingScreenStyle == LoadingScreenStyle::DisclaimerBootSequence)
    {
        if (m_loadingCompletedAtSeconds < 0.0f)
        {
            return bootSequencePresentationEndTimeSeconds();
        }

        return std::max(m_loadingCompletedAtSeconds, bootSequencePresentationEndTimeSeconds());
    }

    if (m_loadingScreenStyle != LoadingScreenStyle::Disclaimer)
    {
        if (m_loadingCompletedAtSeconds < 0.0f)
        {
            return m_minimumDurationSeconds;
        }

        return std::max(m_loadingCompletedAtSeconds, m_minimumDurationSeconds);
    }

    if (m_loadingCompletedAtSeconds < 0.0f)
    {
        return m_minimumDurationSeconds;
    }

    return std::max(m_loadingCompletedAtSeconds,
                    std::max(m_minimumDurationSeconds - kDisclaimerFadeDurationSeconds, 0.0f)) +
           kDisclaimerFadeDurationSeconds;
}

void LoadingRuntime::setProgress(Application& application, float progress, const char* phase)
{
    m_activationProgress = std::clamp(progress, 0.0f, 1.0f);
    m_progressPhase = phase != nullptr ? phase : "loading";
    refreshOverlay();
    updateProgressTitle(application);
}

bool LoadingRuntime::shouldRenderPreparedPreview() const
{
    return false;
}

void LoadingRuntime::refreshBootSequenceOverlayIfNeeded()
{
    if (m_loadingScreenStyle != LoadingScreenStyle::DisclaimerBootSequence)
    {
        return;
    }

    if (m_elapsedSeconds < kBootSequenceDisclaimerDurationSeconds)
    {
        if (m_bootSequenceVisibleLineRevision == -1)
        {
            return;
        }

        refreshOverlay();
        return;
    }

    const int visibleLineCount = bootSequenceVisibleLineCount();
    const bool cursorVisible = bootSequenceCursorVisible();
    const BootSequenceTextEntry statusText = bootSequenceStatusText();
    if (visibleLineCount == m_bootSequenceVisibleLineRevision &&
        cursorVisible == m_bootSequenceCursorRevision &&
        bootSequenceTextEntriesEqual(statusText, m_bootSequenceStatusRevision))
    {
        return;
    }

    refreshOverlay();
}

void LoadingRuntime::refreshOverlay()
{
    if (m_loadingScreenStyle == LoadingScreenStyle::Disclaimer)
    {
        m_loadingOverlay = StartupFlowOverlay::createDisclaimer();
        return;
    }

    if (m_loadingScreenStyle == LoadingScreenStyle::DisclaimerBootSequence)
    {
        if (m_elapsedSeconds < kBootSequenceDisclaimerDurationSeconds)
        {
            m_bootSequenceVisibleLineRevision = -1;
            m_bootSequenceCursorRevision = false;
            m_bootSequenceStatusRevision = BootSequenceTextEntry{};
            m_loadingOverlay = StartupFlowOverlay::createDisclaimer();
            return;
        }

        m_bootSequenceVisibleLineRevision = bootSequenceVisibleLineCount();
        m_bootSequenceCursorRevision = bootSequenceCursorVisible();
        m_bootSequenceStatusRevision = bootSequenceStatusText();
        m_loadingOverlay = StartupFlowOverlay::createBootSequence(
            visibleBootSequenceLines(), m_bootSequenceCursorRevision, m_bootSequenceStatusRevision);
        return;
    }

    const int percent = static_cast<int>(m_activationProgress * 100.0f + 0.5f);
    m_loadingOverlay =
        StartupFlowOverlay::createLoadingProgress(m_nextRuntimeLabel, percent, m_progressPhase);
}

void LoadingRuntime::updateProgressTitle(Application& application) const
{
    if (m_loadingScreenStyle == LoadingScreenStyle::DisclaimerBootSequence)
    {
        application.setStatusWindowTitle(m_loadingCompletedAtSeconds < 0.0f
                                             ? "EngineStarter - Starting workstation"
                                             : "EngineStarter - Preparing workspace");
        return;
    }

    const float dwellProgress =
        m_minimumDurationSeconds > 0.0f
            ? std::clamp(m_elapsedSeconds / m_minimumDurationSeconds, 0.0f, 1.0f)
            : 1.0f;
    const float visibleProgress =
        m_activationProgress + (1.0f - m_activationProgress) * dwellProgress;
    const int percent = static_cast<int>(visibleProgress * 100.0f + 0.5f);

    std::ostringstream titleStream;
    titleStream << "EngineStarter - Loading " << m_nextRuntimeLabel << " - " << percent << "%";
    if (!m_progressPhase.empty())
    {
        titleStream << " - " << m_progressPhase;
    }

    application.setStatusWindowTitle(titleStream.str());
}
} // namespace engine
