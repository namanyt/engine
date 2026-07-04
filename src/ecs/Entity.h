#pragma once

#include <cstdint>

namespace engine::ecs
{
/// @brief Lightweight opaque entity handle (32-bit unsigned integer).
///
/// Entities are sequential identifiers starting from 1. The value `0`
/// is reserved as an invalid/null sentinel (`kInvalidEntity`).
using Entity = std::uint32_t;

/// @brief Sentinel value representing an invalid or unassigned entity.
inline constexpr Entity kInvalidEntity = 0;
} // namespace engine::ecs
