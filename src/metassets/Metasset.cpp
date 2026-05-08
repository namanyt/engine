#include "metassets/Metasset.h"

namespace engine
{
Metasset::Metasset(std::string name) : m_name(std::move(name)) {}

Metasset::~Metasset() = default;

const std::string& Metasset::name() const noexcept
{
    return m_name;
}
} // namespace engine
