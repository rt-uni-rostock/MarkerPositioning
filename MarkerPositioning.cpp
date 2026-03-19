// MarkerPositioning.cpp: entry point for the MarkerPositioning application.
//

#include "MarkerPositioning.h"
// #include "MainThreadsManager/MainThreadsManager.h"
#include "LiveSupervisorMode.h"
#include "SettingsReader.h"
#include "ImageSource/ImageSourceFactory.h"
#include "ImageSource/ImageSourceConfig.h"
#include "DetectionPipeline.h"
#include "DetectionPipelineConfig.h"
#include "Sink.h"
#include "SinkConfig.h"
#include "PipelineResult.h"
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

using namespace std;


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

    {

        //LOG_INFO("Checking GStreamer environment variables:");

        // Gstreamer evironment variables for debugging
        //const char* gst_path = std::getenv("GST_PLUGIN_PATH");
        //const char* path = std::getenv("PATH");

        //if (gst_path)
        //    LOG_INFO("GST_PLUGIN_PATH = {}", gst_path);
        //else
        //    LOG_WARN("GST_PLUGIN_PATH is not set");

        //if (path)
        //    LOG_INFO("PATH = {}", path);
        //else
        //    LOG_WARN("PATH is not set");


        LOG_INFO("Loading settings from JSON file...");

        // Load settings from JSON file
        SettingsReader settingsReader("Settings.json");
        const GeneralSettings& settings = settingsReader.get();
		std::vector<const CameraSettings*> activeCameraSettings = settingsReader.getActiveCameraSettingsList();

        LOG_INFO("Settings loaded successfully.");
        LOG_TRACE("Loaded settings: sourceMode={}, tagType={}, tagSize={}, tagID={}, quadDecimate={}, udpIp={}, udpPort={}, frameRate={}",
            static_cast<int>(settings.sourceMode), static_cast<int>(settings.tagType), settings.tagSize, settings.tagID, settings.quadDecimate, settings.udpIp,
            settings.udpPort, settings.frameRate);

        
        // Soon: for each camera init ImageSourceConfig
        // Now: only first camera from ImageSourceConfig


        LOG_INFO("Initializing ImageSource 1...");

        ImageSourceConfig sourceConfig1;
        const CameraSettings& cam1 = *activeCameraSettings[0];
		sourceConfig1.mode = settings.sourceMode;
		sourceConfig1.cameraSettings = &cam1;
		sourceConfig1.maxCaptureFPS = settings.frameRate * 3; // set max capture FPS to the frame rate specified in settings
        
		LOG_INFO("Selecting ImageSource1 based on settings: mode={}, streamType={}", static_cast<int>(sourceConfig1.mode), static_cast<int>(sourceConfig1.cameraSettings->streamType));

        ImageSourceFactory source1(sourceConfig1);
		auto imgSource1 = source1.create();

        LOG_INFO("ImageSource1 successfully initialized.");

        LOG_INFO("Initializing DetectionPipeline...");

        DetectionPipelineConfig pipelineConfig;
		pipelineConfig.cx = cam1.cx;
		pipelineConfig.cy = cam1.cy;
		pipelineConfig.fx = cam1.fx;
		pipelineConfig.fy = cam1.fy;
		pipelineConfig.d1 = cam1.d1;
		pipelineConfig.d2 = cam1.d2;
		pipelineConfig.d3 = cam1.d3;
		pipelineConfig.d4 = cam1.d4;
		pipelineConfig.d5 = cam1.d5;
		pipelineConfig.tagSize = settings.tagSize;
		pipelineConfig.tagID = settings.tagID;
		pipelineConfig.quadDecimate = settings.quadDecimate;
		pipelineConfig.detectionType = (settings.tagType == TagType::AprilTag) ? DetectionType::AprilTag : DetectionType::ArUco;
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
        
        // Create ImageSource vector
		std::vector<IImageSource*> imgSources = { imgSource1.get() };

        LiveSupervisorMode supervisor(imgSources, pipeline, sink, settings);

		LOG_INFO("Supervisor successfully initialized.");
		LOG_INFO("Starting Supervisor...");

        bool supervisorStartedSuccessfully = supervisor.start();

        if (supervisorStartedSuccessfully) {
            LOG_INFO("Supervisor started successfully.");
            LOG_INFO("Application is running. Press Ctrl+C to shut down...");

            waitForShutdownSignal();

            LOG_INFO("Shutdown signal received, stopping Supervisor...");

            // TODO: warum supervisor stop bevor sink?
            bool supervisorStopped = supervisor.stop();
        }
        else {
			LOG_CRITICAL("Failed to start Supervisor, shutting down application.");
        }

    }

	LOG_INFO("Application shutting down...");

    Logger::shutdown();

	return 0;

}
