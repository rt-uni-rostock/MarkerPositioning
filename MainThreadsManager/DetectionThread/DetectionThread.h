#pragma once

#include <thread>
#include <vector>
#include <mutex>
#include <opencv2/opencv.hpp>

#include "../../SettingsReader/SettingsReader.h"
#include "../../ImageReceiverThread/FrameData.h"

#include "AprilTagDetection/AprilTagDetection.h"

class DetectionThread
{
public:
	DetectionThread(SettingsReader::MainSettings settings);
	~DetectionThread();

	void startDetectionThread(FrameData latestFrame);
	void stopDetectionThread();
	bool isRunning();

	Pose getResults()
	{
		std::scoped_lock lock(resultMutex);
		return results;
	}
private:
	SettingsReader::MainSettings Settings;
	AprilTagDetection Detection;

	void run(std::stop_token st);

	std::jthread detectionThread;
	std::atomic<bool> running = false;

	std::mutex resultMutex;
	Pose results;

	FrameData currentFrame;

	cv::Mat preprocessFrame(cv::Mat rawFrame);
};

