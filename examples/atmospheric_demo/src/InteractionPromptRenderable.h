#pragma once

#include "primitives/Quad.h"
#include "systems/RenderSystem.h"

#include "StartupFlowOverlay.h"

#include <string>
#include <unordered_map>

namespace engine
{
class Camera;
class ShaderLibrary;

class InteractionPromptRenderable final
{
  public:
    void prepare(ShaderLibrary& shaderLibrary);
    void clear() noexcept;
    void clearActivePrompt() noexcept;
    void setActivePrompt(const std::string& promptText);
    bool hasActivePrompt() const noexcept;
    bool hasCachedActivePrompt() const;
    int cacheEntryCount() const noexcept;
    bool ensureActivePromptCached();
    float activePromptWorldHeight() const;
    systems::RenderItem buildRenderItem(const Camera& camera, const Vec3& anchorPosition,
                                        float opacity) const;

  private:
    struct CachedPromptTexture final
    {
        StartupFlowOverlay texture;
        float aspectRatio = 1.0f;
        float pixelWidth = 0.0f;
        float pixelHeight = 0.0f;
    };

    static std::string buildCacheKey(const std::string& promptText);
    const CachedPromptTexture* activePromptTexture() const;

    const Shader* m_shader = nullptr;
    Material m_material{};
    Quad m_quad;
    std::string m_activePromptText;
    std::string m_activePromptKey;
    std::unordered_map<std::string, CachedPromptTexture> m_promptCache;
};
} // namespace engine
