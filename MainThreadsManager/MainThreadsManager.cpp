#include "MainThreadsManager.h"
#include <iostream>
#include <cmath>

using steady_clock = std::chrono::steady_clock;

// Constructor: initializes the main threads with the given settings
MainThreadsManager::MainThreadsManager(SettingsReader::MainSettings settings) : ImageReceiver(settings), Detection(settings), UDPSender(settings)
{

	Settings = settings;

	startImageReceiver();

	triggerProcessing();
}

MainThreadsManager::~MainThreadsManager()
{
	// Destructor implementation
}

// Main loop that triggers processing of images and sending of UDP packets
void MainThreadsManager::triggerProcessing()
{
	// calculate interval based on frame rate
	double interval = round(1000.0 / Settings.frameRate);

	// convert interval to milliseconds
	const std::chrono::milliseconds interval_ms((long) interval);

	// initialize next stop, where the detection should be finished
	auto nextStop = steady_clock::now();

	// initialize next run, where the next detection should start
	auto nextRun = steady_clock::now() + std::chrono::milliseconds(200);

	// main detection loop
	while (true) {

		// check if image receiver is still running
		if (!ImageReceiver.running) {
			break;
		}

		// update next stop and next run times
		nextStop += interval_ms;
		nextRun += interval_ms;

		// log current time
		std::cerr << "[MainThreadsManager] Detection loop at " << std::chrono::duration_cast<std::chrono::milliseconds>(steady_clock::now().time_since_epoch()).count() << " ms" << std::endl;

		// get latest frame from image receiver
		FrameData latestFrame = ImageReceiver.getLatestFrame();

		// start detection thread with latest frame
		Detection.startDetectionThread(latestFrame);

		// sleep until next stop
		std::this_thread::sleep_until(nextStop);

		// check if detection thread is running, get results and send via UDP
		if (Detection.isRunning()) {
			Detection.stopDetectionThread();
			
			Pose data = Detection.getResults();
			std::cout << "[MainThreadsManager] Thread delivered result: z=" << data.z << std::endl;

			// send data via UDP
			UDPSender.startSending(data);
		}

		// sleep until next run
		std::this_thread::sleep_until(nextRun);
	}
}

// Starts the image receiving thread
void MainThreadsManager::startImageReceiver()
{
	ImageReceiver.startReceiving();
}