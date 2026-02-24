#include "Logger.h"

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

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

#ifdef NDEBUG
    console_sink->set_level(spdlog::level::warn);   // Release
#else
    console_sink->set_level(spdlog::level::debug);  // Debug
#endif

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