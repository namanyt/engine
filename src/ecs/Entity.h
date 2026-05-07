#pragma once

#include <cstdint>

namespace engine::ecs
{
using Entity = std::uint32_t;

inline constexpr Entity kInvalidEntity = 0;
} // namespace engine::ecs
