#include "InteractionPromptRenderable.h"

#include "core/ShaderLibrary.h"
#include "world/Camera.h"

#include <algorithm>
#include <string_view>

namespace
{
constexpr std::string_view kPromptShaderKey = "runtime.interaction_prompt";
constexpr std::string_view kPromptStyleKey = "default";
constexpr float kPromptWorldHeight = 0.18f;
constexpr float kPromptCameraOffset = 0.08f;
constexpr float kPromptVerticalLift = 0.06f;

engine::Mat4 makeUprightBillboardMatrix(const engine::Vec3& center,
                                        const engine::Vec3& faceDirection, float width,
                                        float height)
{
    const engine::Vec3 worldUp{0.0f, 1.0f, 0.0f};
    const engine::Vec3 forward = engine::length(faceDirection) > 0.0001f
                                     ? engine::normalize(faceDirection)
                                     : engine::Vec3{0.0f, 0.0f, 1.0f};
    engine::Vec3 right = engine::cross(worldUp, forward);
    if (engine::length(right) <= 0.0001f)
    {
        right = engine::Vec3{1.0f, 0.0f, 0.0f};
    }
    else
    {
        right = engine::normalize(right);
    }

    const engine::Vec3 up = engine::normalize(engine::cross(forward, right));

    engine::Mat4 matrix = engine::Mat4::identity();
    matrix.elements[0] = right.x * width;
    matrix.elements[1] = right.y * width;
    matrix.elements[2] = right.z * width;
    matrix.elements[4] = up.x * height;
    matrix.elements[5] = up.y * height;
    matrix.elements[6] = up.z * height;
    matrix.elements[8] = forward.x;
    matrix.elements[9] = forward.y;
    matrix.elements[10] = forward.z;
    matrix.elements[12] = center.x;
    matrix.elements[13] = center.y;
    matrix.elements[14] = center.z;
    return matrix;
}
} // namespace

namespace engine
{
void InteractionPromptRenderable::prepare(ShaderLibrary& shaderLibrary)
{
    if (m_shader == nullptr)
    {
        m_shader = &shaderLibrary.loadGraphicsProgram(std::string{kPromptShaderKey},
                                                      std::string{"interaction_prompt.vert"},
                                                      std::string{"interaction_prompt.frag"});
        m_material.shader = m_shader;
        m_material.albedo = Vec3{1.0f, 1.0f, 1.0f};
        m_material.emissiveColor = Vec3{1.0f, 1.0f, 1.0f};
        m_material.emissiveStrength = 1.0f;
        m_material.baseEmissiveStrength = 1.0f;
        m_material.atmosphericResponse = 0.0f;
        m_material.roughness = 1.0f;
        m_material.specularStrength = 0.0f;
    }
}

void InteractionPromptRenderable::clear() noexcept
{
    m_promptCache.clear();
    m_activePromptText.clear();
    m_activePromptKey.clear();
}

void InteractionPromptRenderable::clearActivePrompt() noexcept
{
    m_activePromptText.clear();
    m_activePromptKey.clear();
}

void InteractionPromptRenderable::setActivePrompt(const std::string& promptText)
{
    if (promptText.empty())
    {
        clearActivePrompt();
        return;
    }

    m_activePromptText = promptText;
    m_activePromptKey = buildCacheKey(promptText);
}

bool InteractionPromptRenderable::hasActivePrompt() const noexcept
{
    return !m_activePromptKey.empty();
}

bool InteractionPromptRenderable::hasCachedActivePrompt() const
{
    return activePromptTexture() != nullptr;
}

int InteractionPromptRenderable::cacheEntryCount() const noexcept
{
    return static_cast<int>(m_promptCache.size());
}

bool InteractionPromptRenderable::ensureActivePromptCached()
{
    if (m_shader == nullptr || !hasActivePrompt())
    {
        return false;
    }

    if (m_promptCache.find(m_activePromptKey) != m_promptCache.end())
    {
        return false;
    }

    CachedPromptTexture texture{};
    texture.texture = StartupFlowOverlay::createInteractionPromptTexture(m_activePromptText);
    texture.pixelWidth = static_cast<float>(texture.texture.width());
    texture.pixelHeight = static_cast<float>(texture.texture.height());
    texture.aspectRatio =
        texture.pixelHeight > 0.0f ? texture.pixelWidth / texture.pixelHeight : 1.0f;
    m_promptCache.emplace(m_activePromptKey, std::move(texture));
    return true;
}

float InteractionPromptRenderable::activePromptWorldHeight() const
{
    const CachedPromptTexture* promptTexture = activePromptTexture();
    if (promptTexture == nullptr)
    {
        return 0.0f;
    }

    (void)promptTexture;
    return kPromptWorldHeight;
}

systems::RenderItem InteractionPromptRenderable::buildRenderItem(const Camera& camera,
                                                                 const Vec3& anchorPosition,
                                                                 float opacity) const
{
    systems::RenderItem item{};
    const CachedPromptTexture* promptTexture = activePromptTexture();
    if (promptTexture == nullptr || m_shader == nullptr || opacity <= 0.001f)
    {
        item.visible = false;
        return item;
    }

    const float worldHeight = activePromptWorldHeight();
    const float worldWidth = worldHeight * promptTexture->aspectRatio;
    Vec3 toCamera = camera.position() - anchorPosition;
    toCamera.y = 0.0f;
    const Vec3 facingDirection =
        length(toCamera) > 0.0001f ? normalize(toCamera) : Vec3{0.0f, 0.0f, 1.0f};
    const Vec3 promptCenter = anchorPosition + facingDirection * kPromptCameraOffset +
                              Vec3{0.0f, kPromptVerticalLift + worldHeight * 0.5f, 0.0f};

    item.debugName = "InteractionPrompt";
    item.mesh = &m_quad.mesh();
    item.modelMatrix =
        makeUprightBillboardMatrix(promptCenter, facingDirection, worldWidth, worldHeight);
    item.material = m_material;
    item.textureId = promptTexture->texture.textureId();
    item.opacity = std::clamp(opacity, 0.0f, 1.0f);
    item.castsShadows = false;
    item.visible = true;
    item.alphaBlended = true;
    item.depthTest = true;
    item.depthWrite = false;
    item.doubleSided = true;
    item.worldUi = true;
    return item;
}

std::string InteractionPromptRenderable::buildCacheKey(const std::string& promptText)
{
    return std::string{kPromptStyleKey} + ':' + promptText;
}

const InteractionPromptRenderable::CachedPromptTexture*
InteractionPromptRenderable::activePromptTexture() const
{
    if (!hasActivePrompt())
    {
        return nullptr;
    }

    const auto iterator = m_promptCache.find(m_activePromptKey);
    return iterator != m_promptCache.end() ? &iterator->second : nullptr;
}
} // namespace engine
