#include "systems/InteractionSystem.h"

#include "components/WorldComponents.h"
#include "runtime/OverlayUiLayout.h"
#include "world/Camera.h"
#include "world/Scene.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kFocusStickyBias = 0.18f;
constexpr float kPromptLift = 0.9f;

engine::Vec3
interactionFocusTarget(const engine::components::TransformComponent& transform) noexcept
{
    return transform.position;
}

bool isValidFocusCandidate(const engine::Camera& camera,
                           const engine::components::TransformComponent& transform,
                           const engine::components::InteractableComponent& interactable,
                           float& score, engine::Vec3& anchor) noexcept
{
    if (!interactable.enabled || interactable.interactionRadius <= 0.0f)
    {
        return false;
    }

    anchor = engine::systems::interactionFocusAnchor(transform, interactable);
    const engine::Vec3 toTarget = interactionFocusTarget(transform) - camera.position();
    const float distance = engine::length(toTarget);
    if (distance <= 0.001f || distance > interactable.interactionRadius)
    {
        return false;
    }

    const engine::Vec3 direction = toTarget / distance;
    const float alignment = engine::dot(camera.front(), direction);
    if (alignment < engine::systems::kInteractionFocusDotThreshold)
    {
        return false;
    }

    score = alignment * 4.0f - distance * 0.12f;
    return true;
}
} // namespace

namespace engine::systems
{
InteractionUpdateResult updateInteraction(Scene& scene, const Camera& camera, bool interactPressed,
                                          InteractionState& state)
{
    InteractionUpdateResult result{};
    ecs::Entity bestEntity = ecs::kInvalidEntity;
    float bestScore = -1000000.0f;
    Vec3 bestAnchor{};

    scene.registry().forEach<components::TransformComponent, components::InteractableComponent>(
        [&](ecs::Entity entity, const components::TransformComponent& transform,
            const components::InteractableComponent& interactable)
        {
            float score = 0.0f;
            Vec3 anchor{};
            if (!isValidFocusCandidate(camera, transform, interactable, score, anchor))
            {
                return;
            }

            if (entity == state.focusedEntity)
            {
                score += kFocusStickyBias;
            }

            if (score > bestScore)
            {
                bestScore = score;
                bestEntity = entity;
                bestAnchor = anchor;
            }
        });

    result.focusChanged = bestEntity != state.focusedEntity;
    state.focusedEntity = bestEntity;
    result.focusedEntity = bestEntity;
    result.promptWorldPosition = bestAnchor;

    if (bestEntity == ecs::kInvalidEntity)
    {
        return result;
    }

    const components::InteractableComponent* interactable =
        scene.registry().tryGet<components::InteractableComponent>(bestEntity);
    if (interactable == nullptr)
    {
        return result;
    }

    result.interactionPrompt = interactable->interactionPrompt;
    result.interactionId = interactable->interactionId;
    result.interactionTriggered = interactPressed && interactable->enabled;
    return result;
}

Vec3 interactionFocusAnchor(const components::TransformComponent& transform,
                            const components::InteractableComponent& interactable) noexcept
{
    const float scaleLift = std::max(transform.scale.y * 0.55f, 0.7f);
    return transform.position +
           Vec3{0.0f, scaleLift + interactable.interactionRadius * 0.05f + kPromptLift, 0.0f};
}

bool projectInteractionPromptToDesignSpace(const Camera& camera, int framebufferWidth,
                                           int framebufferHeight, const Vec3& worldPosition,
                                           Vec2& designPosition) noexcept
{
    const float safeWidth = static_cast<float>(std::max(framebufferWidth, 1));
    const float safeHeight = static_cast<float>(std::max(framebufferHeight, 1));
    const float aspectRatio = safeWidth / safeHeight;
    const Vec3 offset = worldPosition - camera.position();

    const float cameraSpaceX = dot(offset, camera.right());
    const float cameraSpaceY = dot(offset, camera.up());
    const float cameraSpaceZ = dot(offset, camera.front());
    if (cameraSpaceZ <= camera.nearPlane())
    {
        return false;
    }

    const float tangent = std::tan(camera.fieldOfViewRadians() * 0.5f);
    if (tangent <= 0.0001f)
    {
        return false;
    }

    const float ndcX = cameraSpaceX / (cameraSpaceZ * tangent * aspectRatio);
    const float ndcY = cameraSpaceY / (cameraSpaceZ * tangent);
    if (std::abs(ndcX) > 1.1f || std::abs(ndcY) > 1.1f)
    {
        return false;
    }

    designPosition.x = (ndcX * 0.5f + 0.5f) * overlayui::kDesignWidth;
    designPosition.y = (0.5f - ndcY * 0.5f) * overlayui::kDesignHeight;
    return true;
}
} // namespace engine::systems
