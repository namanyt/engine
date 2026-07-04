#include "geometry/ObjParser.h"

#include <cmath>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
constexpr float kEpsilon = 1.0e-4f;

bool approxEqual(float a, float b)
{
    return std::abs(a - b) <= kEpsilon;
}

bool approxEqual(const engine::Vec3& a, const engine::Vec3& b)
{
    return approxEqual(a.x, b.x) && approxEqual(a.y, b.y) && approxEqual(a.z, b.z);
}

bool approxEqual(const engine::Vec2& a, const engine::Vec2& b)
{
    return approxEqual(a.x, b.x) && approxEqual(a.y, b.y);
}

// ============================================================================
// Basic Triangle Tests
// ============================================================================

int obj_parse_simple_triangle()
{
    // Minimal triangle: 3 vertices, 1 face
    const std::string obj = R"(
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 0.5 1.0 0.0
f 1 2 3
)";

    auto geo = engine::ObjParser::parse(obj);
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse simple triangle.\n";
        return 1;
    }

    if (geo->vertices.size() != 3)
    {
        std::cerr << "Expected 3 vertices, got " << geo->vertices.size() << ".\n";
        return 1;
    }

    if (geo->indices.size() != 3)
    {
        std::cerr << "Expected 3 indices, got " << geo->indices.size() << ".\n";
        return 1;
    }

    if (!approxEqual(geo->vertices[0].position, engine::Vec3{0.0f, 0.0f, 0.0f}))
    {
        std::cerr << "Vertex 0 position mismatch.\n";
        return 1;
    }

    if (!approxEqual(geo->vertices[1].position, engine::Vec3{1.0f, 0.0f, 0.0f}))
    {
        std::cerr << "Vertex 1 position mismatch.\n";
        return 1;
    }

    if (!approxEqual(geo->vertices[2].position, engine::Vec3{0.5f, 1.0f, 0.0f}))
    {
        std::cerr << "Vertex 2 position mismatch.\n";
        return 1;
    }

    return 0;
}

int obj_parse_triangle_with_normals()
{
    const std::string obj = R"(
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 0.5 1.0 0.0
vn 0.0 0.0 1.0
f 1//1 2//1 3//1
)";

    auto geo = engine::ObjParser::parse(obj);
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse triangle with normals.\n";
        return 1;
    }

    for (const auto& v : geo->vertices)
    {
        if (!approxEqual(v.normal, engine::Vec3{0.0f, 0.0f, 1.0f}))
        {
            std::cerr << "Normal mismatch.\n";
            return 1;
        }
    }

    return 0;
}

int obj_parse_triangle_with_uvs()
{
    const std::string obj = R"(
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 0.5 1.0 0.0
vt 0.0 0.0
vt 1.0 0.0
vt 0.5 1.0
f 1/1 2/2 3/3
)";

    auto geo = engine::ObjParser::parse(obj);
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse triangle with UVs.\n";
        return 1;
    }

    if (!approxEqual(geo->vertices[0].uv, engine::Vec2{0.0f, 0.0f}))
    {
        std::cerr << "UV 0 mismatch.\n";
        return 1;
    }

    if (!approxEqual(geo->vertices[1].uv, engine::Vec2{1.0f, 0.0f}))
    {
        std::cerr << "UV 1 mismatch.\n";
        return 1;
    }

    if (!approxEqual(geo->vertices[2].uv, engine::Vec2{0.5f, 1.0f}))
    {
        std::cerr << "UV 2 mismatch.\n";
        return 1;
    }

    return 0;
}

int obj_parse_triangle_with_full_refs()
{
    const std::string obj = R"(
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 0.5 1.0 0.0
vt 0.0 0.0
vt 1.0 0.0
vt 0.5 1.0
vn 0.0 1.0 0.0
f 1/1/1 2/2/1 3/3/1
)";

    auto geo = engine::ObjParser::parse(obj);
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse triangle with full vertex refs.\n";
        return 1;
    }

    if (geo->vertices.size() != 3)
    {
        std::cerr << "Expected 3 vertices, got " << geo->vertices.size() << ".\n";
        return 1;
    }

    for (const auto& v : geo->vertices)
    {
        if (!approxEqual(v.normal, engine::Vec3{0.0f, 1.0f, 0.0f}))
        {
            std::cerr << "Normal mismatch in full ref test.\n";
            return 1;
        }
    }

    return 0;
}

// ============================================================================
// Quad and N-Gon Tests
// ============================================================================

int obj_parse_quad()
{
    const std::string obj = R"(
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 1.0 1.0 0.0
v 0.0 1.0 0.0
f 1 2 3 4
)";

    auto geo = engine::ObjParser::parse(obj);
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse quad.\n";
        return 1;
    }

    // Quad should be triangulated into 2 triangles (6 vertices, 6 indices)
    if (geo->indices.size() != 6)
    {
        std::cerr << "Expected 6 indices for quad, got " << geo->indices.size() << ".\n";
        return 1;
    }

    if (geo->vertices.size() != 6)
    {
        std::cerr << "Expected 6 vertices for quad, got " << geo->vertices.size() << ".\n";
        return 1;
    }

    return 0;
}

int obj_parse_pentagon()
{
    const std::string obj = R"(
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 1.0 1.0 0.0
v 0.5 1.5 0.0
v 0.0 1.0 0.0
f 1 2 3 4 5
)";

    auto geo = engine::ObjParser::parse(obj);
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse pentagon.\n";
        return 1;
    }

    // Pentagon → 3 triangles (9 vertices, 9 indices)
    if (geo->indices.size() != 9)
    {
        std::cerr << "Expected 9 indices for pentagon, got " << geo->indices.size() << ".\n";
        return 1;
    }

    return 0;
}

// ============================================================================
// Edge Cases
// ============================================================================

int obj_parse_empty_input()
{
    auto geo = engine::ObjParser::parse(std::string{});
    if (geo.has_value())
    {
        std::cerr << "Empty input should return nullopt.\n";
        return 1;
    }

    return 0;
}

int obj_parse_comments_only()
{
    const std::string obj = R"(
# This is a comment
# Another comment
)";

    auto geo = engine::ObjParser::parse(obj);
    if (geo.has_value())
    {
        std::cerr << "Comments-only input should return nullopt.\n";
        return 1;
    }

    return 0;
}

int obj_parse_negative_indices()
{
    const std::string obj = R"(
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 0.5 1.0 0.0
f -3 -2 -1
)";

    auto geo = engine::ObjParser::parse(obj);
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse negative indices.\n";
        return 1;
    }

    // -3 → vertex 1 (index 0), -2 → vertex 2 (index 1), -1 → vertex 3 (index 2)
    if (!approxEqual(geo->vertices[0].position, engine::Vec3{0.0f, 0.0f, 0.0f}))
    {
        std::cerr << "Negative index wrap-around failed for vertex 0.\n";
        return 1;
    }

    if (!approxEqual(geo->vertices[1].position, engine::Vec3{1.0f, 0.0f, 0.0f}))
    {
        std::cerr << "Negative index wrap-around failed for vertex 1.\n";
        return 1;
    }

    if (!approxEqual(geo->vertices[2].position, engine::Vec3{0.5f, 1.0f, 0.0f}))
    {
        std::cerr << "Negative index wrap-around failed for vertex 2.\n";
        return 1;
    }

    return 0;
}

int obj_parse_multiple_faces()
{
    const std::string obj = R"(
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 1.0 1.0 0.0
v 2.0 0.0 0.0
v 2.0 1.0 0.0
f 1 2 3
f 2 4 5
)";

    auto geo = engine::ObjParser::parse(obj);
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse multiple faces.\n";
        return 1;
    }

    // 2 triangles → 6 vertices, 6 indices
    if (geo->vertices.size() != 6)
    {
        std::cerr << "Expected 6 vertices for two triangles, got " << geo->vertices.size() << ".\n";
        return 1;
    }

    if (geo->indices.size() != 6)
    {
        std::cerr << "Expected 6 indices for two triangles, got " << geo->indices.size() << ".\n";
        return 1;
    }

    return 0;
}

int obj_parse_ignores_comments_and_whitespace()
{
    const std::string obj = R"(
# Comment line

v   0.0   0.0   0.0
v   1.0   0.0   0.0
  # Indented comment
v   0.5   1.0   0.0

f   1   2   3
)";

    auto geo = engine::ObjParser::parse(obj);
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse OBJ with comments and extra whitespace.\n";
        return 1;
    }

    if (geo->vertices.size() != 3)
    {
        std::cerr << "Expected 3 vertices, got " << geo->vertices.size() << ".\n";
        return 1;
    }

    return 0;
}

int obj_parse_cube()
{
    // Cube: 8 unique vertices, 12 triangular faces (2 triangles per side)
    const std::string cube = R"(
v -1.0 -1.0 -1.0
v  1.0 -1.0 -1.0
v  1.0  1.0 -1.0
v -1.0  1.0 -1.0
v -1.0 -1.0  1.0
v  1.0 -1.0  1.0
v  1.0  1.0  1.0
v -1.0  1.0  1.0
f 1 2 3
f 1 3 4
f 5 6 7
f 5 7 8
f 1 5 8
f 1 8 4
f 2 6 7
f 2 7 3
f 5 1 2
f 5 2 6
f 4 8 7
f 4 7 3
)";

    auto geo = engine::ObjParser::parse(cube);
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse cube.\n";
        return 1;
    }

    // 12 triangles → 36 vertices, 36 indices
    if (geo->vertices.size() != 36)
    {
        std::cerr << "Expected 36 vertices for cube, got " << geo->vertices.size() << ".\n";
        return 1;
    }

    if (geo->indices.size() != 36)
    {
        std::cerr << "Expected 36 indices for cube, got " << geo->indices.size() << ".\n";
        return 1;
    }

    return 0;
}

int obj_parse_default_normal()
{
    // Vertices without explicit normals should get default (0, 1, 0)
    const std::string obj = R"(
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 0.5 1.0 0.0
f 1 2 3
)";

    auto geo = engine::ObjParser::parse(obj);
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse OBJ with default normals.\n";
        return 1;
    }

    for (const auto& v : geo->vertices)
    {
        if (!approxEqual(v.normal, engine::Vec3{0.0f, 1.0f, 0.0f}))
        {
            std::cerr << "Default normal should be (0, 1, 0).\n";
            return 1;
        }
    }

    return 0;
}

int obj_parse_from_bytes()
{
    const std::string text = R"(
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 0.5 1.0 0.0
f 1 2 3
)";

    auto geo =
        engine::ObjParser::parse(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
    if (!geo.has_value())
    {
        std::cerr << "Failed to parse OBJ from byte buffer.\n";
        return 1;
    }

    if (geo->vertices.size() != 3)
    {
        std::cerr << "Expected 3 vertices from byte buffer, got " << geo->vertices.size() << ".\n";
        return 1;
    }

    return 0;
}

int obj_parse_null_input()
{
    auto geo = engine::ObjParser::parse(nullptr, 0);
    if (geo.has_value())
    {
        std::cerr << "Null input should return nullopt.\n";
        return 1;
    }

    return 0;
}

int obj_parse_vertices_without_faces()
{
    const std::string obj = R"(
v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 0.5 1.0 0.0
)";

    auto geo = engine::ObjParser::parse(obj);
    if (geo.has_value())
    {
        std::cerr << "Vertices without faces should return nullopt.\n";
        return 1;
    }

    return 0;
}

// ============================================================================
// Dispatch Table
// ============================================================================

struct NamedTest
{
    std::string_view name;
    int (*function)();
};
} // namespace

int main(int argc, char** argv)
{
    const std::vector<NamedTest> tests = {
        {"obj_parse_simple_triangle", &obj_parse_simple_triangle},
        {"obj_parse_triangle_with_normals", &obj_parse_triangle_with_normals},
        {"obj_parse_triangle_with_uvs", &obj_parse_triangle_with_uvs},
        {"obj_parse_triangle_with_full_refs", &obj_parse_triangle_with_full_refs},
        {"obj_parse_quad", &obj_parse_quad},
        {"obj_parse_pentagon", &obj_parse_pentagon},
        {"obj_parse_empty_input", &obj_parse_empty_input},
        {"obj_parse_comments_only", &obj_parse_comments_only},
        {"obj_parse_negative_indices", &obj_parse_negative_indices},
        {"obj_parse_multiple_faces", &obj_parse_multiple_faces},
        {"obj_parse_ignores_comments_and_whitespace", &obj_parse_ignores_comments_and_whitespace},
        {"obj_parse_cube", &obj_parse_cube},
        {"obj_parse_default_normal", &obj_parse_default_normal},
        {"obj_parse_from_bytes", &obj_parse_from_bytes},
        {"obj_parse_null_input", &obj_parse_null_input},
        {"obj_parse_vertices_without_faces", &obj_parse_vertices_without_faces}};

    if (argc != 2)
    {
        std::cerr << "Usage: ObjParserTests <test-name>\n";
        return 1;
    }

    const std::string_view requestedTest = argv[1];

    for (const NamedTest& test : tests)
    {
        if (test.name == requestedTest)
        {
            return test.function();
        }
    }

    std::cerr << "Unknown test: " << requestedTest << '\n';
    return 1;
}
