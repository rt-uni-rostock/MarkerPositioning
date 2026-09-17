#include "Logger.h"

#include <cstdlib>
#include <string>

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace
{
    // Reads the desired console log level from the MP_CONSOLE_LOG_LEVEL
    // environment variable (e.g. "trace", "debug", "info", "warn", "err",
    // "critical", "off"). Falls back to spdlog::level::info if the variable
    // is unset or contains an unrecognized value.
    spdlog::level::level_enum consoleLevelFromEnv()
    {
        const char* envValue = std::getenv("MP_CONSOLE_LOG_LEVEL");
        if (envValue == nullptr)
        {
            return spdlog::level::info;
        }

        const spdlog::level::level_enum parsedLevel = spdlog::level::from_str(envValue);
        // spdlog::level::from_str() returns off for both an explicit "off"
        // and any unrecognized string, so re-check to detect the latter and
        // fall back to the default instead of silently muting the console.
        if (parsedLevel == spdlog::level::off && std::string(envValue) != "off")
        {
            return spdlog::level::info;
        }

        return parsedLevel;
    }
}

void Logger::init()
{
    // -----------------------------------------
    // 1️⃣ Async Thread Pool
    // -----------------------------------------
    // Queue size = 8192 messages
    // 1 dedicated logging thread
    spdlog::init_thread_pool(8192, 1);

    // -----------------------------------------
    // 2️⃣ File Sink (logs everything)
    // -----------------------------------------
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/app.log",
        1024 * 1024 * 5,  // 5 MB
        3                 // 3 rotating files
    );

    file_sink->set_level(spdlog::level::trace);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] [thread %t] %v");

    // -----------------------------------------
    // 3️⃣ Console Sink
    // -----------------------------------------
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // Console verbosity is intentionally independent of the CMAKE_BUILD_TYPE
    // (Debug/Release, i.e. NDEBUG) so behavior is consistent across platforms
    // and build presets (e.g. Windows Debug vs. Linux Release presets).
    // Defaults to "info"; override at runtime with the MP_CONSOLE_LOG_LEVEL
    // environment variable, e.g.:
    //   MP_CONSOLE_LOG_LEVEL=debug ./MarkerPositioning
    console_sink->set_level(consoleLevelFromEnv());

    console_sink->set_pattern("[%H:%M:%S] [%^%l%$] %v");

    // -----------------------------------------
    // 4️⃣ Async Logger
    // -----------------------------------------
    auto async_logger = std::make_shared<spdlog::async_logger>(
        "default_async_logger",
        spdlog::sinks_init_list{ file_sink, console_sink },
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block
    );

    async_logger->set_level(spdlog::level::trace);

    // 🔥 Set as global default logger
    spdlog::set_default_logger(async_logger);
}

void Logger::shutdown()
{
    spdlog::shutdown(); // flush + stop thread pool
}