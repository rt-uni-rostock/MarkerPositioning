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
        LOG_INFO("Loading settings from JSON file...");

        // Load settings from JSON file
        SettingsReader settingsReader("Settings.json");
        const GeneralSettings& settings = settingsReader.get();
		std::vector<const CameraSettings*> activeCameraSettings = settingsReader.getActiveCameraSettingsList();

        LOG_INFO("Settings loaded successfully.");
        LOG_TRACE("Loaded settings: sourceMode={}, tagType={}, tagSize={}, tagID={}, quadDecimate={}, udpIp={}, udpPort={}, frameRate={}",
            static_cast<int>(settings.sourceMode), static_cast<int>(settings.tagType), settings.tagSize, settings.tagID, settings.quadDecimate, settings.udpIp,
            settings.udpPort, settings.frameRate);


		LOG_INFO("Initializing ImageSources...");

		// Vector to hold all initialized ImageSources, will be passed to supervisor
		std::vector<std::unique_ptr<IImageSource>> imgSources;

		// Vector to hold all initialized DetectionPipelines, will be passed to supervisor
		std::vector<std::unique_ptr<DetectionPipeline>> pipelines;

		// Loop through all active cameras and initialize corresponding ImageSources

		size_t numActiveCameras = activeCameraSettings.size();

        for (size_t i = 0; i < numActiveCameras; i++) {

            ImageSourceConfig sourceConfig;
            const CameraSettings& cam = *activeCameraSettings[i];

            sourceConfig.mode = settings.sourceMode;
            sourceConfig.cameraSettings = &cam;
            sourceConfig.maxCaptureFPS = settings.frameRate * 3; // set max capture FPS to the frame rate specified in settings
            
            LOG_INFO("Selecting ImageSource {} based on settings: mode={}, streamType={}", i + 1, static_cast<int>(sourceConfig.mode), static_cast<int>(sourceConfig.cameraSettings->streamType));
            ImageSourceFactory sourceFactory(sourceConfig);
            auto imgSource = sourceFactory.create();
            LOG_INFO("ImageSource {} successfully initialized.", i + 1);

			// Add the initialized ImageSource to the vector
			imgSources.push_back(std::move(imgSource));

            DetectionPipelineConfig pipelineConfig;
            pipelineConfig.cameraId = cam.id;
            pipelineConfig.cx = cam.cx;
            pipelineConfig.cy = cam.cy;
            pipelineConfig.fx = cam.fx;
            pipelineConfig.fy = cam.fy;
            pipelineConfig.d1 = cam.d1;
            pipelineConfig.d2 = cam.d2;
            pipelineConfig.d3 = cam.d3;
            pipelineConfig.d4 = cam.d4;
            pipelineConfig.d5 = cam.d5;

            if (cam.id == 2 || cam.id == 3) {
                pipelineConfig.tagSize = 0.12;
                pipelineConfig.tagID = 17;
            }
            else if (cam.id == 4) {
                pipelineConfig.tagSize = 0.75;
                pipelineConfig.tagID = 11;
                /*pipelineConfig.tagSize = 0.12;
                pipelineConfig.tagID = 17;*/
            }
            else {
                pipelineConfig.tagSize = settings.tagSize;
                pipelineConfig.tagID = settings.tagID;
            }

            /*pipelineConfig.tagSize = settings.tagSize;
            pipelineConfig.tagID = settings.tagID;*/
            pipelineConfig.quadDecimate = settings.quadDecimate;
            pipelineConfig.detectionType = (settings.tagType == TagType::AprilTag) ? DetectionType::AprilTag : DetectionType::ArUco;
            
            // Image Logging Konfiguration von GeneralSettings übernehmen
            pipelineConfig.enableImageLogging = settings.enableImageLogging;
            pipelineConfig.imageOutputPath = settings.imageOutputPath;
            pipelineConfig.saveRawFrames = settings.imageLogOptions.saveRawFrames;
            pipelineConfig.saveGrayFrames = settings.imageLogOptions.saveGrayFrames;
            pipelineConfig.saveDetectionResults = settings.imageLogOptions.saveDetectionResults;
            pipelineConfig.visualizeAllDetections = settings.imageLogOptions.visualizeAllDetections;
            
            auto pipeline = std::make_unique<DetectionPipeline>(std::move(pipelineConfig));

            pipelines.push_back(std::move(pipeline));
		}

        // Erstelle Vektor mit Raw-Pointern für den Supervisor
        std::vector<IImageSource*> imgSourcePtrs;
        for (auto& src : imgSources) {
            imgSourcePtrs.push_back(src.get());
        }

		std::vector<DetectionPipeline*> pipelinePtrs;
        for (auto& pipe : pipelines) {
            pipelinePtrs.push_back(pipe.get());
		}


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

        LiveSupervisorMode supervisor(imgSourcePtrs, pipelinePtrs, sink, settings);

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
