#pragma once

#include <memory>
#include <spdlog/spdlog.h>

class Logger
{
public:
    static void init();
    static void shutdown();
};

//
// Projekt-Makros (benutzen Default Logger)
// Diese können compile-time entfernt werden
//

#define LOG_TRACE(...)    SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)