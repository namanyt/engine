#include "geometry/ObjParser.h"

#include <sstream>

namespace engine
{
namespace
{
/// @brief Converts a 1-based OBJ index (possibly negative) to a 0-based C++ index.
/// Returns -1 if idx is 0 (invalid in OBJ) or out of range.
int toZeroBased(int idx, std::size_t listSize)
{
    if (idx == 0)
        return -1; // OBJ indices are 1-based; 0 is invalid

    if (idx < 0)
        idx = static_cast<int>(listSize) + idx;
    else
        --idx;

    if (idx < 0 || idx >= static_cast<int>(listSize))
        return -1;

    return idx;
}
} // namespace

std::optional<Geometry> ObjParser::parse(const std::uint8_t* data, std::size_t size)
{
    if (!data || size == 0)
        return std::nullopt;

    std::string text(reinterpret_cast<const char*>(data), size);
    return parse(text);
}

std::optional<Geometry> ObjParser::parse(const std::string& text)
{
    ObjParser parser;

    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line))
    {
        // Trim leading whitespace
        auto start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
            continue;
        line = line.substr(start);

        // Skip comments and empty lines
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream tokenStream(line);
        char cmdChar = line[0];

        if (cmdChar == 'v' && line.size() > 1 && line[1] == ' ')
        {
            RawVertex vert;
            float x, y, z;
            std::string cmd;
            tokenStream >> cmd >> x >> y >> z;
            if (!tokenStream.fail())
            {
                vert.position = Vec3{x, y, z};
                parser.m_vertices.push_back(vert);
            }
        }
        else if (cmdChar == 'v' && line.size() > 1 && line[1] == 'n')
        {
            float x, y, z;
            std::string cmd;
            tokenStream >> cmd >> x >> y >> z;
            if (!tokenStream.fail())
            {
                parser.m_normals.push_back(Vec3{x, y, z});
            }
        }
        else if (cmdChar == 'v' && line.size() > 1 && line[1] == 't')
        {
            float u, v;
            std::string cmd;
            tokenStream >> cmd >> u >> v;
            if (!tokenStream.fail())
            {
                parser.m_uvs.push_back(Vec2{u, v});
            }
        }
        else if (cmdChar == 'f' && line.size() > 1 && line[1] == ' ')
        {
            // Parse face vertex references inline (FaceRef is a private nested type)
            std::vector<FaceRef> face;
            std::string cmd;
            tokenStream >> cmd; // Skip "f"

            while (true)
            {
                FaceRef ref{};
                if (!(tokenStream >> ref.vIndex))
                    break;

                if (tokenStream.peek() == '/')
                {
                    tokenStream.ignore(1);
                    // Could be "v//vn" or "v/vt/vn"
                    if (tokenStream.peek() != '/')
                    {
                        tokenStream >> ref.vtIndex;
                    }

                    if (tokenStream.peek() == '/')
                    {
                        tokenStream.ignore(1);
                        tokenStream >> ref.vnIndex;
                    }
                }

                face.push_back(ref);
            }

            if (face.size() < 3)
                continue;

            // Lambda to resolve a FaceRef into a Vertex using parser's data members
            auto resolve = [&](const FaceRef& fr) -> Vertex
            {
                Vertex out;

                int vIdx = toZeroBased(fr.vIndex, parser.m_vertices.size());
                if (vIdx >= 0)
                    out.position = parser.m_vertices[vIdx].position;

                if (fr.vnIndex != 0)
                {
                    int nIdx = toZeroBased(fr.vnIndex, parser.m_normals.size());
                    if (nIdx >= 0)
                        out.normal = parser.m_normals[nIdx];
                }
                else
                {
                    // Fallback: use default normal from raw vertex
                    if (vIdx >= 0)
                        out.normal = parser.m_vertices[vIdx].normal;
                }

                if (fr.vtIndex != 0)
                {
                    int uvIdx = toZeroBased(fr.vtIndex, parser.m_uvs.size());
                    if (uvIdx >= 0)
                        out.uv = parser.m_uvs[uvIdx];
                }
                else
                {
                    // Fallback: use default UV from raw vertex
                    if (vIdx >= 0)
                        out.uv = parser.m_vertices[vIdx].uv;
                }

                return out;
            };

            // Fan-triangulate from first vertex
            for (std::size_t i = 1; i + 1 < face.size(); ++i)
            {
                unsigned int baseIdx = static_cast<unsigned int>(parser.m_geometry.vertices.size());

                parser.m_geometry.vertices.push_back(resolve(face[0]));
                parser.m_geometry.vertices.push_back(resolve(face[i]));
                parser.m_geometry.vertices.push_back(resolve(face[i + 1]));

                parser.m_geometry.indices.push_back(baseIdx);
                parser.m_geometry.indices.push_back(baseIdx + 1);
                parser.m_geometry.indices.push_back(baseIdx + 2);
            }
        }
        // Ignore: o, g, s, mtllib, usemtl, etc.
    }

    if (parser.m_geometry.vertices.empty() || parser.m_geometry.indices.empty())
        return std::nullopt;

    return parser.m_geometry;
}
} // namespace engine
