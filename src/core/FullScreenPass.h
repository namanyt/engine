#pragma once

namespace engine
{
class FullScreenPass final
{
  public:
    FullScreenPass();
    ~FullScreenPass();

    FullScreenPass(const FullScreenPass&) = delete;
    FullScreenPass& operator=(const FullScreenPass&) = delete;
    FullScreenPass(FullScreenPass&&) = delete;
    FullScreenPass& operator=(FullScreenPass&&) = delete;

    void draw() const;

  private:
    unsigned int m_vertexArrayId = 0;
    unsigned int m_vertexBufferId = 0;
  };
} // namespace engine
