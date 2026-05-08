#pragma once

#include <string>

namespace engine
{
class Metasset
{
  public:
    explicit Metasset(std::string name);
    virtual ~Metasset();

    Metasset(const Metasset&) = delete;
    Metasset& operator=(const Metasset&) = delete;
    Metasset(Metasset&&) = delete;
    Metasset& operator=(Metasset&&) = delete;

    const std::string& name() const noexcept;

  private:
    std::string m_name;
};
} // namespace engine
