#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace engine
{
enum class VnCommandType
{
    Background,
    CharacterSet,
    Character,
    HideCharacter,
    Name,
    Text,
    Wait,
    Fade,
    Music,
    Sfx,
    End,
};

enum class VnStageRegion
{
    Left,
    Center,
    Right,
};

struct VnInstruction final
{
    VnCommandType type = VnCommandType::Text;
    int lineNumber = 0;
    std::string identifier;
    std::filesystem::path assetPath;
    std::string text;
    VnStageRegion stageRegion = VnStageRegion::Center;
    float xOffsetNormalized = 0.0f;
    float yOffsetNormalized = 0.0f;
    float scale = 1.0f;
    float durationSeconds = 0.0f;
};

struct VnScript final
{
    std::vector<VnInstruction> instructions;
};

VnScript parseVnScriptFile(const std::filesystem::path& scriptPath);
VnScript parseVnScriptText(std::string_view scriptText,
                           const std::filesystem::path& sourcePath = {});
} // namespace engine
