#include "core/RenderDebug.h"

#include <glad/glad.h>

#include <algorithm>

namespace
{
constexpr unsigned int kMaxDebugLabelLength = 255;

bool supportsDebugMarkers() noexcept
{
    return glPushDebugGroup != nullptr && glPopDebugGroup != nullptr;
}

bool supportsLabels() noexcept
{
    return glObjectLabel != nullptr;
}
} // namespace

namespace engine
{
bool supportsRenderDebugGroups() noexcept
{
    return supportsDebugMarkers();
}

bool supportsObjectLabels() noexcept
{
    return supportsLabels();
}

void pushRenderDebugGroup(std::string_view label) noexcept
{
    if (!supportsDebugMarkers() || label.empty())
    {
        return;
    }

    const int labelLength =
        static_cast<int>(std::min<std::size_t>(label.size(), kMaxDebugLabelLength));
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, labelLength, label.data());
}

void popRenderDebugGroup() noexcept
{
    if (!supportsDebugMarkers())
    {
        return;
    }

    glPopDebugGroup();
}

void labelGlObject(unsigned int identifier, unsigned int objectId, std::string_view label) noexcept
{
    if (!supportsLabels() || objectId == 0 || label.empty())
    {
        return;
    }

    const int labelLength =
        static_cast<int>(std::min<std::size_t>(label.size(), kMaxDebugLabelLength));
    glObjectLabel(identifier, objectId, labelLength, label.data());
}

ScopedRenderDebugGroup::ScopedRenderDebugGroup(std::string_view label) noexcept
    : m_active(supportsDebugMarkers() && !label.empty())
{
    if (m_active)
    {
        pushRenderDebugGroup(label);
    }
}

ScopedRenderDebugGroup::~ScopedRenderDebugGroup()
{
    if (m_active)
    {
        popRenderDebugGroup();
    }
}
} // namespace engine
