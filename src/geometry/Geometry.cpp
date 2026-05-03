#include "geometry/Geometry.h"

namespace engine
{
bool Geometry::empty() const noexcept
{
    return vertices.empty() || indices.empty();
}
} // namespace engine
