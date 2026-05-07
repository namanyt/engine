#pragma once

#include "world/Scene.h"

struct GLFWwindow;

namespace engine
{
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
    void draw(Scene& scene);
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
