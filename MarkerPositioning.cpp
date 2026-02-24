// MarkerPositioning.cpp: entry point for the MarkerPositioning application.
//

#include "MarkerPositioning.h"
// #include "MainThreadsManager/MainThreadsManager.h"
#include "LiveSupervisorMode.h"
#include "SettingsReader.h"
#include "ImageSourceFactory.h"
#include "ImageSourceConfig.h"
#include "DetectionPipeline.h"
#include "DetectionPipelineConfig.h"
#include "Sink.h"
#include "SinkConfig.h"
#include "PipelineResult.h"

// shutdown includes
#include <atomic>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>

#include "Logger.h"

using namespace std;


//// Shutdown handling

// indicates whether a shutdown signal was requested (e.g. Ctrl+C)
std::atomic<bool> shutdownRequested(false);

// condition variable and mutex for waiting for shutdown signal
static std::mutex shutdownMutex;
static std::condition_variable shutdownCV;

// signal handler for shutdown signals (e.g. SIGINT)
// SIGINT: Interrupt from keyboard (e.g. Ctrl+C)
// SIGTERM: Termination signal from system (e.g. kill command)
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        shutdownRequested.store(true);

		// Notify the main thread to proceed with shutdown
        shutdownCV.notify_one();
    }
}

// Function to wait for a shutdown signal (e.g. Ctrl+C)
void waitForShutdownSignal() {
    std::unique_lock<std::mutex> lock(shutdownMutex);
    shutdownCV.wait(lock, [] { return shutdownRequested.load(); });
}


//// Main function: entry point of the application

int main()
{

    Logger::init();

	LOG_INFO("Application starting...");

    {

        LOG_INFO("Checking GStreamer environment variables:");

        // Gstreamer evironment variables for debugging
        const char* gst_path = std::getenv("GST_PLUGIN_PATH");
        const char* path = std::getenv("PATH");

        if (gst_path)
            LOG_INFO("GST_PLUGIN_PATH = {}", gst_path);
        else
            LOG_WARN("GST_PLUGIN_PATH is not set");

        if (path)
            LOG_INFO("PATH = {}", path);
        else
            LOG_WARN("PATH is not set");


        LOG_INFO("Loading settings from JSON file...");

        // Load settings from JSON file
        SettingsReader settingsReader("Settings.json");
        const MainSettings& settings = settingsReader.get();

        LOG_INFO("Settings loaded successfully.");
        LOG_TRACE("Loaded settings: sourceMode={} streamType={}, tagType={}, tagSize={}, tagID={}, fx={}, fy={}, cx={}, cy={}, d1={}, d2={}, d3={}, d4={}, d5={}, quadDecimate={}, rtspUrl={}, udpIp={}, udpPort={}, frameRate={}",
            static_cast<int>(settings.sourceMode), static_cast<int>(settings.streamType), static_cast<int>(settings.tagType), settings.tagSize, settings.tagID, settings.fx, settings.fy, settings.cx,
            settings.cy, settings.d1, settings.d2, settings.d3, settings.d4, settings.d5, settings.quadDecimate, settings.rtspUrl, settings.udpIp,
            settings.udpPort, settings.frameRate);

        //OLD: Start main threads manager, which handles all threads, including image receiving, detection, and UDP sending
        //OLD: MainThreadsManager mainThreadsManager = MainThreadsManager(settings);

        LOG_INFO("Initializing ImageSource...");

        ImageSourceConfig sourceConfig;
		sourceConfig.mode = settings.sourceMode;
		sourceConfig.type = settings.streamType;
		sourceConfig.rtspUrl = settings.rtspUrl;
        // TODO: static files path in settings
		sourceConfig.filePath = "C:/path/to/images"; // for recorded mode, path to image files
        
		LOG_INFO("Selecting ImageSource based on settings: mode={}, type={}", static_cast<int>(sourceConfig.mode), static_cast<int>(sourceConfig.type));

        ImageSourceFactory source(sourceConfig);
		auto imgSource = source.create();

        LOG_INFO("ImageSource successfully initialized.");
        LOG_INFO("Initializing DetectionPipeline...");

        DetectionPipelineConfig pipelineConfig;
        // TODO: configure pipelineConfig based on settings if needed
        DetectionPipeline pipeline(pipelineConfig);

        LOG_INFO("DetectionPipeline successfully initialized.");
        LOG_INFO("Initializing Sink...");

        SinkConfig sinkConfig;
        sinkConfig.udpEnabled = true; // enable UDP sending
        sinkConfig.loggingEnabled = true; // enable logging to SQLite
        sinkConfig.udpAddress = settings.udpIp;
        sinkConfig.udpPort = settings.udpPort;
        sinkConfig.sqliteFilePath = "results.db"; // path to SQLite database file
        sinkConfig.batchSize = 10; // number of log events to batch in one transaction
        sinkConfig.batchTimeoutMs = 500; // maximum time to wait for batch to fill up before writing to SQLite

        Sink sink(sinkConfig);

        LOG_INFO("Sink successfully initialized.");



        // Read Settings, determine which supervisor mode to use, and run the corresponding mode
        // Test here: live mode
		
        LOG_INFO("Live Supervisor Mode selected based on settings, initializing supervisor...");
        
        LiveSupervisorMode supervisor(*imgSource, pipeline, sink, settings);

		LOG_INFO("Supervisor successfully initialized.");
		LOG_INFO("Starting Supervisor...");

        //supervisor.start();

		LOG_INFO("Supervisor started successfully.");
		LOG_INFO("Application is running. Press Ctrl+C to shut down...");
        
        //waitForShutdownSignal();

		LOG_INFO("Shutdown signal received, stopping Supervisor...");

        //supervisor.stop();

		LOG_INFO("Supervisor stopped successfully.");

        LOG_INFO("Starting Sink...");

        sink.start();

        LOG_INFO("Sink successfully started.");

        LOG_INFO("Sending test results to Sink...");

        for (int i = 0; i < 10; ++i)
        {
            PipelineResult result;
            auto now = std::chrono::system_clock::now();
            result.imageTimestamp = std::format("{:%FT%TZ}", now);

            result.markerId = i;
            result.cameraId = 1;
            result.markerType = 42;
            result.errorCode = 0;
            result.errorMessage = "";

            result.posX = 1.0f * i;
            result.posY = 2.0f * i;
            result.posZ = 3.0f * i;

            result.rotX = 0.1f;
            result.rotY = 0.2f;
            result.rotZ = 0.3f;

            sink.send(result);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        LOG_INFO("Finished sending test results.");

        // ---- Let workers process ----
        std::this_thread::sleep_for(std::chrono::seconds(1));

        LOG_INFO("Stopping Sink...");

        sink.stop();

        LOG_INFO("Sink stopped.");

    }

	LOG_INFO("Application shutting down...");

    Logger::shutdown();

	return 0;

}
