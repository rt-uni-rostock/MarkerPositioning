// MarkerPositioning.cpp: entry point for the MarkerPositioning application.
//

#include "MarkerPositioning.h"
// #include "MainThreadsManager/MainThreadsManager.h"
#include "ISupervisorMode.h"
#include "SupervisorFactory.h"
#include "SettingsReader.h"
#include "GeneralSettings.h"

// shutdown includes
#include <atomic>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>

#include "Logger.h"

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/chrono.h>

//// Shutdown handling

// indicates whether a shutdown signal was requested (e.g. Ctrl+C)
std::atomic<bool> shutdownRequested(false);

// signal handler for shutdown signals (e.g. SIGINT)
// SIGINT: Interrupt from keyboard (e.g. Ctrl+C)
// SIGTERM: Termination signal from system (e.g. kill command)
void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        shutdownRequested.store(true);
    }
}

// Function to wait for a shutdown signal (e.g. Ctrl+C)
void waitForShutdownSignal()
{
    while (!shutdownRequested.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


//// Main function: entry point of the application

int main()
{

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    Logger::init();

	LOG_INFO("Application starting...");

    try
    {
        LOG_INFO("Loading settings from JSON file...");

        // Load settings from JSON file
        SettingsReader settingsReader("Settings.json");
        const GeneralSettings& settings = settingsReader.get();

        LOG_INFO("Settings loaded successfully.");
        LOG_TRACE("Loaded settings: supervisorMode={}, sourceMode={}, tagType={}, tagSize={}, tagID={}, quadDecimate={}, udpIp={}, udpPort={}, frameRate={}",
            static_cast<int>(settings.supervisorMode), static_cast<int>(settings.sourceMode), static_cast<int>(settings.tagType), settings.tagSize, settings.tagID, settings.quadDecimate, settings.udpIp,
            settings.udpPort, settings.frameRate);
		
        LOG_INFO("Initializing supervisor via SupervisorFactory...");
        SupervisorFactory supervisorFactory(settings);
        auto supervisor = supervisorFactory.create();

        LOG_INFO("Supervisor successfully initialized.");
        LOG_INFO("Starting Supervisor...");

        bool supervisorStartedSuccessfully = supervisor->start();

        if (supervisorStartedSuccessfully) {
            LOG_INFO("Supervisor started successfully.");
            LOG_INFO("Application is running. Press Ctrl+C to shut down...");

            waitForShutdownSignal();

            LOG_INFO("Shutdown signal received, stopping Supervisor...");

            try {
                supervisor->stop();
                LOG_INFO("Supervisor stopped successfully.");
            }
            catch (const std::exception& e) {
                LOG_ERROR("Exception during supervisor shutdown: {}. Continuing with cleanup...", e.what());
            }
            catch (...) {
                LOG_ERROR("Unknown exception during supervisor shutdown. Continuing with cleanup...");
            }
        }
        else {
        	LOG_CRITICAL("Failed to start Supervisor, shutting down application.");
        }

    }
    catch (const std::exception& e)
    {
        LOG_CRITICAL("Unhandled exception: {}. Shutting down application.", e.what());
        Logger::shutdown();
        return 1;
    }
    catch (...)
    {
        LOG_CRITICAL("Unknown unhandled exception. Shutting down application.");
        Logger::shutdown();
        return 1;
    }

	LOG_INFO("Application shutting down...");

    Logger::shutdown();

	return 0;

}
