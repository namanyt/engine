#pragma once

#include <string_view>

namespace engine
{
bool supportsRenderDebugGroups() noexcept;
bool supportsObjectLabels() noexcept;

void pushRenderDebugGroup(std::string_view label) noexcept;
void popRenderDebugGroup() noexcept;
void labelGlObject(unsigned int identifier, unsigned int objectId, std::string_view label) noexcept;

class ScopedRenderDebugGroup final
{
  public:
    explicit ScopedRenderDebugGroup(std::string_view label) noexcept;
    ~ScopedRenderDebugGroup();

    ScopedRenderDebugGroup(const ScopedRenderDebugGroup&) = delete;
    ScopedRenderDebugGroup& operator=(const ScopedRenderDebugGroup&) = delete;
    ScopedRenderDebugGroup(ScopedRenderDebugGroup&&) = delete;
    ScopedRenderDebugGroup& operator=(ScopedRenderDebugGroup&&) = delete;

  private:
    bool m_active = false;
};
} // namespace engine
