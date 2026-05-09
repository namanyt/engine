#include "runtime/VnScript.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace engine
{
namespace
{
std::string trim(std::string_view text)
{
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0)
    {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
    {
        --end;
    }

    return std::string{text.substr(start, end - start)};
}

bool startsWith(std::string_view text, std::string_view prefix)
{
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

std::string takeToken(std::string_view& text)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0)
    {
        text.remove_prefix(1);
    }

    std::size_t index = 0;
    while (index < text.size() && std::isspace(static_cast<unsigned char>(text[index])) == 0)
    {
        ++index;
    }

    const std::string token{text.substr(0, index)};
    text.remove_prefix(index);
    return token;
}

float parseFloat(std::string_view text, const std::filesystem::path& sourcePath, int lineNumber,
                 const char* fieldName)
{
    const std::string trimmed = trim(text);
    float value = 0.0f;
    const char* begin = trimmed.data();
    const char* end = trimmed.data() + trimmed.size();
    const std::from_chars_result result = std::from_chars(begin, end, value);
    if (result.ec == std::errc{} && result.ptr == end)
    {
        return value;
    }

    std::ostringstream stream;
    stream << "Failed to parse " << fieldName << " on line " << lineNumber;
    if (!sourcePath.empty())
    {
        stream << " of " << sourcePath.string();
    }
    stream << '.';
    throw std::runtime_error(stream.str());
}

std::string unescapeText(std::string_view text)
{
    std::string output;
    output.reserve(text.size());
    bool escaping = false;
    for (const char character : text)
    {
        if (!escaping)
        {
            if (character == '\\')
            {
                escaping = true;
                continue;
            }

            output.push_back(character);
            continue;
        }

        switch (character)
        {
        case 'n':
            output.push_back('\n');
            break;
        case 't':
            output.push_back('\t');
            break;
        case '\\':
            output.push_back('\\');
            break;
        default:
            output.push_back(character);
            break;
        }

        escaping = false;
    }

    if (escaping)
    {
        output.push_back('\\');
    }

    return output;
}

[[noreturn]] void throwParseError(const std::filesystem::path& sourcePath, int lineNumber,
                                  const std::string& message)
{
    std::ostringstream stream;
    stream << "VN script parse error on line " << lineNumber;
    if (!sourcePath.empty())
    {
        stream << " of " << sourcePath.string();
    }
    stream << ": " << message;
    throw std::runtime_error(stream.str());
}

VnStageRegion parseStageRegion(std::string_view text, const std::filesystem::path& sourcePath,
                               int lineNumber)
{
    const std::string lowered = trim(text);
    if (lowered == "left")
    {
        return VnStageRegion::Left;
    }
    if (lowered == "center")
    {
        return VnStageRegion::Center;
    }
    if (lowered == "right")
    {
        return VnStageRegion::Right;
    }

    throwParseError(sourcePath, lineNumber,
                    "CHARACTER stage region must be left, center, or right.");
}
} // namespace

VnScript parseVnScriptFile(const std::filesystem::path& scriptPath)
{
    std::ifstream input(scriptPath);
    if (!input)
    {
        throw std::runtime_error("Failed to open VN script: " + scriptPath.string());
    }

    std::ostringstream stream;
    stream << input.rdbuf();
    return parseVnScriptText(stream.str(), scriptPath);
}

VnScript parseVnScriptText(std::string_view scriptText, const std::filesystem::path& sourcePath)
{
    VnScript script;
    std::size_t lineStart = 0;
    int lineNumber = 0;
    while (lineStart <= scriptText.size())
    {
        const std::size_t lineEnd = scriptText.find('\n', lineStart);
        const std::string_view rawLine = lineEnd == std::string_view::npos
                                             ? scriptText.substr(lineStart)
                                             : scriptText.substr(lineStart, lineEnd - lineStart);
        lineStart = lineEnd == std::string_view::npos ? scriptText.size() + 1 : lineEnd + 1;
        ++lineNumber;

        std::string line = trim(rawLine);
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty() || startsWith(line, "#") || startsWith(line, "//"))
        {
            continue;
        }

        std::string_view remaining = line;
        const std::string command = takeToken(remaining);
        const std::string payload = trim(remaining);

        VnInstruction instruction{};
        instruction.lineNumber = lineNumber;

        if (command == "BACKGROUND")
        {
            if (payload.empty())
            {
                throwParseError(sourcePath, lineNumber, "BACKGROUND requires an image path.");
            }

            instruction.type = VnCommandType::Background;
            instruction.assetPath = std::filesystem::path{payload};
        }
        else if (command == "CHARACTER_SET")
        {
            std::string_view arguments = payload;
            instruction.identifier = takeToken(arguments);
            const std::string imagePath = trim(arguments);
            if (instruction.identifier.empty() || imagePath.empty())
            {
                throwParseError(sourcePath, lineNumber,
                                "CHARACTER_SET requires an id and image path.");
            }

            instruction.type = VnCommandType::CharacterSet;
            instruction.assetPath = std::filesystem::path{imagePath};
        }
        else if (command == "CHARACTER")
        {
            std::string_view arguments = payload;
            instruction.identifier = takeToken(arguments);
            const std::string stageRegion = takeToken(arguments);
            const std::string xOffset = takeToken(arguments);
            const std::string yOffset = takeToken(arguments);
            const std::string scale = takeToken(arguments);
            if (instruction.identifier.empty() || stageRegion.empty() || xOffset.empty() ||
                yOffset.empty() || scale.empty())
            {
                throwParseError(
                    sourcePath, lineNumber,
                    "CHARACTER requires id, stage region, x offset, y offset, and scale.");
            }

            instruction.type = VnCommandType::Character;
            instruction.stageRegion = parseStageRegion(stageRegion, sourcePath, lineNumber);
            instruction.xOffsetNormalized =
                parseFloat(xOffset, sourcePath, lineNumber, "CHARACTER x offset");
            instruction.yOffsetNormalized =
                parseFloat(yOffset, sourcePath, lineNumber, "CHARACTER y offset");
            instruction.scale = parseFloat(scale, sourcePath, lineNumber, "CHARACTER scale");
        }
        else if (command == "HIDE_CHARACTER")
        {
            if (payload.empty())
            {
                throwParseError(sourcePath, lineNumber, "HIDE_CHARACTER requires an id.");
            }

            instruction.type = VnCommandType::HideCharacter;
            instruction.identifier = payload;
        }
        else if (command == "NAME")
        {
            instruction.type = VnCommandType::Name;
            instruction.text = unescapeText(payload);
        }
        else if (command == "TEXT")
        {
            instruction.type = VnCommandType::Text;
            instruction.text = unescapeText(payload);
        }
        else if (command == "WAIT")
        {
            if (payload.empty())
            {
                throwParseError(sourcePath, lineNumber, "WAIT requires a duration in seconds.");
            }

            instruction.type = VnCommandType::Wait;
            instruction.durationSeconds =
                parseFloat(payload, sourcePath, lineNumber, "WAIT duration");
        }
        else if (command == "FADE")
        {
            if (payload.empty())
            {
                throwParseError(sourcePath, lineNumber, "FADE requires a duration in seconds.");
            }

            instruction.type = VnCommandType::Fade;
            instruction.durationSeconds =
                parseFloat(payload, sourcePath, lineNumber, "FADE duration");
        }
        else if (command == "MUSIC")
        {
            instruction.type = VnCommandType::Music;
            instruction.text = payload;
        }
        else if (command == "SFX")
        {
            instruction.type = VnCommandType::Sfx;
            instruction.text = payload;
        }
        else if (command == "END")
        {
            instruction.type = VnCommandType::End;
        }
        else
        {
            throwParseError(sourcePath, lineNumber, "Unknown command '" + command + "'.");
        }

        script.instructions.push_back(std::move(instruction));
    }

    return script;
}
} // namespace engine
