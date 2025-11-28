#include "MainThreadsManager.h"
#include <iostream>
#include <cmath>

using steady_clock = std::chrono::steady_clock;

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

void MainThreadsManager::triggerProcessing()
{
	// TODO: variable time interval
	double interval = round(1000.0 / Settings.frameRate);
	std::cout << "[MainThreadsManager] Trigger processing loop started with interval " << interval << " ms." << std::endl;
	long linterval = (long)interval;
	std::cout << "[MainThreadsManager] Rounded interval: " << linterval << " ms." << std::endl;
	const std::chrono::milliseconds interval_ms((long) interval);
	auto nextStop = steady_clock::now();
	auto nextRun = steady_clock::now() + std::chrono::milliseconds(200);

	while (true) {
		if (!ImageReceiver.running) {
			break;
		}

		nextStop += interval_ms;
		nextRun += interval_ms;

		std::cerr << "[MainThreadsManager] Detection loop at " << std::chrono::duration_cast<std::chrono::milliseconds>(steady_clock::now().time_since_epoch()).count() << " ms" << std::endl;

		FrameData latestFrame = ImageReceiver.getLatestFrame();
		Detection.startDetectionThread(latestFrame);

		std::this_thread::sleep_until(nextStop);

		if (Detection.isRunning()) {
			Detection.stopDetectionThread();
			//std::cout << "Detection stopped." << std::endl;

			Pose data = Detection.getResults();
			std::cout << "[MainThreadsManager] Thread delivered result: z=" << data.z << std::endl;

			UDPSender.startSending(data);
		}

		// sleep until next run
		std::this_thread::sleep_until(nextRun);
	}
}

void MainThreadsManager::startImageReceiver()
{
	ImageReceiver.startReceiving();
}