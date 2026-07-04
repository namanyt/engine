#pragma once

#include "geometry/Geometry.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace engine
{
/**
 * @brief Parses a Wavefront OBJ file into a Geometry object.
 *
 * Supports the following OBJ features:
 *   - v (vertex positions)
 *   - vn (vertex normals)
 *   - vt (texture coordinates)
 *   - f (faces with vertex/normal/uv indices)
 *   - o / g (object and group names, used for multi-mesh separation)
 *   - s (smooth shading hints)
 *
 * Triangulation:
 *   Faces with more than 3 vertices are fan-triangulated from the first
 *   vertex. This matches the behaviour of most lightweight OBJ loaders.
 *
 * Indexing:
 *   OBJ uses 1-based indices with optional per-attribute offsets
 *   (e.g. "f 1/2/3 4/5/6 ..."). Missing UV or normal indices default to
 *   the vertex index. Negative indices wrap around the end of the list.
 *
 * @see Geometry
 */
class ObjParser final
{
  public:
    /// @brief Parses OBJ data from a raw byte buffer.
    /// @param data Pointer to the OBJ file contents (may contain null bytes).
    /// @param size Byte count of the buffer.
    /// @return Parsed Geometry, or std::nullopt if parsing failed.
    static std::optional<Geometry> parse(const std::uint8_t* data, std::size_t size);

    /// @brief Parses OBJ data from a UTF-8 string.
    /// @param text Full OBJ file contents as a string.
    /// @return Parsed Geometry, or std::nullopt if parsing failed.
    static std::optional<Geometry> parse(const std::string& text);

  private:
    ObjParser() = default;

    struct RawVertex
    {
        Vec3 position{};
        Vec3 normal{0.0f, 1.0f, 0.0f}; ///< Default up-facing normal.
        Vec2 uv{};
    };

    struct FaceRef
    {
        int vIndex = 0;  ///< 1-based vertex index from OBJ.
        int vtIndex = 0; ///< 1-based UV index (0 = absent).
        int vnIndex = 0; ///< 1-based normal index (0 = absent).
    };

    std::vector<RawVertex> m_vertices;
    std::vector<Vec3> m_normals;
    std::vector<Vec2> m_uvs;
    Geometry m_geometry;
};
} // namespace engine
