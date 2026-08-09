#pragma once

#include <iostream>
#include <string_view>
#include <format>

namespace shlogger
{

enum class Level
{
    debug,
    info,
    warn,
    error
};

void log(Level lvl, std::string_view message);

void set_level(Level lvl);
Level get_level();

template<typename... Args>
void log(Level lvl, std::format_string<Args...> format, Args&&... arguments)
{
    log(lvl, std::format(format, std::forward<Args>(arguments)...));
}

}

#define LOG_DEBUG(...) \
    if constexpr (SHLOGGER_LEVEL <= 0) \
        shlogger::log(shlogger::Level::debug, __VA_ARGS__)

#define LOG_INFO(...) \
    if constexpr (SHLOGGER_LEVEL <= 1) \
        shlogger::log(shlogger::Level::info, __VA_ARGS__)

#define LOG_WARN(...) \
    if constexpr (SHLOGGER_LEVEL <= 2) \
        shlogger::log(shlogger::Level::warn, __VA_ARGS__)

#define LOG_ERROR(...) \
    if constexpr (SHLOGGER_LEVEL <= 3) \
        shlogger::log(shlogger::Level::error, __VA_ARGS__)


#ifdef SHLOGGER_IMPL

#ifndef SHLOGGER_LEVEL
#define SHLOGGER_LEVEL 1
#endif

namespace shlogger
{

namespace
{

Level current_lvl = static_cast<Level>(SHLOGGER_LEVEL);

const char* level_name(Level lvl)
{
    switch (lvl)
    {
        case Level::debug: return "DEBUG";
        case Level::info:  return "INFO ";
        case Level::warn:  return "WARN ";
        case Level::error: return "ERROR";
    }

    return "?????";
}

}

void log(Level lvl, std::string_view message)
{
    if (lvl < current_lvl)
    {
        return;
    }

    std::cerr << '[' << level_name(lvl) << "] " << message << "\n";
}

void set_level(Level lvl)
{
    current_lvl = lvl;
}

Level get_level()
{
    return current_lvl;
}

}

#endif
