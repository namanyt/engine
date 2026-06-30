#include "SceneRuntime.h"

#include "core/Renderer.h"

namespace engine
{
SceneRuntime::SceneRuntime(const SceneMetasset& sceneMetasset) : m_sceneMetasset(sceneMetasset) {}

SceneRuntime::~SceneRuntime() = default;

const SceneMetasset& SceneRuntime::metasset() const noexcept
{
    return m_sceneMetasset;
}

void SceneRuntime::deactivate(Renderer& renderer)
{
    (void)renderer;
}
} // namespace engine
