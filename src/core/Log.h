#pragma once

#include <string_view>

namespace engine
{
enum class LogLevel
{
    info,
    warning,
    error,
};

class Log final
{
public:
    static void write(LogLevel level, std::string_view channel, std::string_view message);

    static void info(std::string_view channel, std::string_view message);
    static void warning(std::string_view channel, std::string_view message);
    static void error(std::string_view channel, std::string_view message);
};
} // namespace engine
