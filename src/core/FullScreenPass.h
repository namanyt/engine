#pragma once

namespace engine
{
/**
 * @brief Pre-allocated VAO/VBO for rendering a full-screen quad.
 *
 * Encapsulates the vertex buffer and array state needed to draw a single
 * triangle-strip quad that covers the entire viewport. Used by post-processing
 * passes that render screen-space effects.
 *
 * @see PostProcessor
 */
class FullScreenPass final
{
  public:
    /// @brief Constructs the full-screen quad VAO and VBO.
    FullScreenPass();

    /// @brief Destroys GPU resources.
    ~FullScreenPass();

    FullScreenPass(const FullScreenPass&) = delete;
    FullScreenPass& operator=(const FullScreenPass&) = delete;
    FullScreenPass(FullScreenPass&&) = delete;
    FullScreenPass& operator=(FullScreenPass&&) = delete;

    /// @brief Draws the full-screen quad using the currently bound shader and texture.
    void draw() const;

  private:
    unsigned int m_vertexArrayId = 0;  ///< OpenGL VAO handle.
    unsigned int m_vertexBufferId = 0; ///< OpenGL VBO handle.
};
} // namespace engine
