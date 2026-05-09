#pragma once

#include "components/WorldComponents.h"
#include "ecs/Entity.h"
#include "math/Types.h"

#include <string>

namespace engine
{
class Camera;
class Scene;
} // namespace engine

namespace engine::systems
{
constexpr float kInteractionFocusDotThreshold = 0.985f;

struct InteractionState final
{
    ecs::Entity focusedEntity = ecs::kInvalidEntity;
};

struct InteractionUpdateResult final
{
    ecs::Entity focusedEntity = ecs::kInvalidEntity;
    Vec3 promptWorldPosition{};
    std::string interactionPrompt;
    std::string interactionId;
    bool focusChanged = false;
    bool interactionTriggered = false;
};

InteractionUpdateResult updateInteraction(Scene& scene, const Camera& camera, bool interactPressed,
                                          InteractionState& state);
Vec3 interactionFocusAnchor(const components::TransformComponent& transform,
                            const components::InteractableComponent& interactable) noexcept;
bool projectInteractionPromptToDesignSpace(const Camera& camera, int framebufferWidth,
                                           int framebufferHeight, const Vec3& worldPosition,
                                           Vec2& designPosition) noexcept;
} // namespace engine::systems
