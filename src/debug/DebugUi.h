#pragma once

#include "world/TestWorld.h"

struct GLFWwindow;

namespace engine
{
struct FramePerformanceStats;
struct RendererDebugTextures;
class Player;
class PlayerController;

struct ExplorationRuntimeStats final
{
    int entityCount = 0;
    int componentTypeCount = 0;
    int componentCount = 0;
};

class DebugUi final
{
  public:
    DebugUi(GLFWwindow* window);
    ~DebugUi();

    DebugUi(const DebugUi&) = delete;
    DebugUi& operator=(const DebugUi&) = delete;
    DebugUi(DebugUi&&) = delete;
    DebugUi& operator=(DebugUi&&) = delete;

    void beginFrame() const;
    void draw(AtmosphericWorldSettings& worldSettings, AtmosphericRenderSettings& renderSettings,
              AtmosphericRuntimeState& runtimeState, const ExplorationRuntimeStats& stats,
              Player& player, PlayerController& playerController, bool& debugFreeCameraEnabled,
              const FramePerformanceStats& performanceStats,
              const RendererDebugTextures& debugTextures);
    void endFrame() const;
    bool consumeResumeCameraRequest() noexcept;
    bool shouldQuit() const noexcept;

    void setEnabled(bool enabled) noexcept;
    bool isEnabled() const noexcept;

  private:
    bool m_enabled = true;
    bool m_shouldQuit = false;
    bool m_shouldResumeCamera = false;
};
} // namespace engine
