#pragma once

#include <filesystem>
#include <string>

namespace engine
{
struct Mat4;

/**
 * @brief OpenGL shader program wrapper.
 *
 * Loads GLSL source files, compiles vertex and fragment stages, links them
 * into a program, and provides helpers for setting uniforms.
 *
 * Shaders are loaded from the filesystem at construction time. Compilation
 * or link failures throw `std::runtime_error` with diagnostic context.
 *
 * @par Example
 * @code
 * Shader shader("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl");
 * shader.use();
 * shader.setMat4("uModel", modelMatrix);
 * @endcode
 *
 * @see ShaderLibrary
 */
class Shader final
{
  public:
    /// @brief Constructs and links a shader program from GLSL source files.
    /// @param vertexShaderPath Path to the vertex shader (.glsl / .vert) file.
    /// @param fragmentShaderPath Path to the fragment shader (.glsl / .frag) file.
    /// @throws std::runtime_error if compilation or linking fails.
    Shader(const std::filesystem::path& vertexShaderPath,
           const std::filesystem::path& fragmentShaderPath);

    /// @brief Destroys the OpenGL program object.
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    /// @brief Binds this shader program as the active program.
    void use() const;

    /// @brief Sets an integer uniform by name.
    /// @param name Uniform variable name in the GLSL source.
    /// @param value Integer value to upload.
    void setInt(const std::string& name, int value) const;

    /// @brief Sets a float uniform by name.
    /// @param name Uniform variable name in the GLSL source.
    /// @param value Float value to upload.
    void setFloat(const std::string& name, float value) const;

    /// @brief Sets a vec2 uniform by name.
    /// @param name Uniform variable name in the GLSL source.
    /// @param x X component.
    /// @param y Y component.
    void setVec2(const std::string& name, float x, float y) const;

    /// @brief Sets a vec3 uniform by name.
    /// @param name Uniform variable name in the GLSL source.
    /// @param x X component.
    /// @param y Y component.
    /// @param z Z component.
    void setVec3(const std::string& name, float x, float y, float z) const;

    /// @brief Sets a mat4 uniform by name.
    /// @param name Uniform variable name in the GLSL source.
    /// @param value 4x4 matrix to upload (column-major).
    void setMat4(const std::string& name, const Mat4& value) const;

    /// @brief Returns the underlying OpenGL program ID.
    /// @return Unsigned integer handle for direct GL calls.
    unsigned int programId() const noexcept;

  private:
    static std::string readTextFile(const std::filesystem::path& path);
    static unsigned int compileStage(unsigned int stage, const std::string& source,
                                     const std::filesystem::path& path);
    static void validateProgramLink(unsigned int programId);

    int uniformLocation(const std::string& name) const;

    unsigned int m_programId = 0;
};
} // namespace engine
