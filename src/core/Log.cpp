#include "core/Log.h"

#include <iostream>

namespace
{
const char* levelLabel(engine::LogLevel level)
{
    switch (level)
    {
    case engine::LogLevel::info:
        return "Info";
    case engine::LogLevel::warning:
        return "Warn";
    case engine::LogLevel::error:
        return "Error";
    }

    return "Log";
}
} // namespace

namespace engine
{
void Log::write(LogLevel level, std::string_view channel, std::string_view message)
{
    std::ostream& stream = level == LogLevel::error ? std::cerr : std::cout;
    stream << '[' << levelLabel(level) << "] [" << channel << "] " << message << '\n';
}

void Log::info(std::string_view channel, std::string_view message)
{
    write(LogLevel::info, channel, message);
}

void Log::warning(std::string_view channel, std::string_view message)
{
    write(LogLevel::warning, channel, message);
}

void Log::error(std::string_view channel, std::string_view message)
{
    write(LogLevel::error, channel, message);
}
} // namespace engine
