/**
 * @file ecs.hpp
 * @brief Public API for the Engine ECS (Entity-Component-System) subsystem.
 *
 * The ECS subsystem provides a lightweight entity-component architecture for
 * managing game objects and their data. Components are plain data structs,
 * and the `Registry` handles storage, retrieval, and iteration.
 *
 * Key types:
 * - `ecs::Entity` — lightweight opaque entity handle (uint32).
 * - `ecs::Registry` — component storage, entity lifecycle, and query engine.
 * - `components::*Component` — world-facing data components (transform, mesh, light, etc.).
 *
 * @par Example
 * @code
 * auto entity = registry.createEntity();
 * registry.emplace<components::TransformComponent>(entity);
 * registry.emplace<components::NameComponent>(entity, "player");
 * @endcode
 *
 * @see ecs::Registry
 * @see components::TransformComponent
 */

#pragma once

#include "ecs/Entity.h"
#include "ecs/Registry.h"
#include "components/WorldComponents.h"
