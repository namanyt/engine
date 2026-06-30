#include "SceneMetasset.h"

namespace engine
{
SceneMetasset::SceneMetasset(std::string name) : Metasset(std::move(name)) {}

SceneMetasset::~SceneMetasset() = default;
} // namespace engine
