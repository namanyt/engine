#include "core/ShaderLibrary.h"

#include "core/Shader.h"

namespace engine
{
ShaderLibrary::ShaderLibrary(std::filesystem::path shaderDirectory)
    : m_shaderDirectory(std::move(shaderDirectory))
{
}

const Shader& ShaderLibrary::loadGraphicsProgram(const std::string& key,
                                                 const std::string& vertexShaderName,
                                                 const std::string& fragmentShaderName)
{
    const auto iterator = m_programs.find(key);
    if (iterator != m_programs.end())
    {
        return *iterator->second;
    }

    auto shader =
        std::make_unique<Shader>(shaderPath(vertexShaderName), shaderPath(fragmentShaderName));
    const Shader& shaderReference = *shader;
    m_programs.emplace(key, std::move(shader));
    return shaderReference;
}

std::filesystem::path ShaderLibrary::shaderPath(const std::string& shaderName) const
{
    return m_shaderDirectory / shaderName;
}

const std::filesystem::path& ShaderLibrary::shaderDirectory() const noexcept
{
    return m_shaderDirectory;
}
} // namespace engine
