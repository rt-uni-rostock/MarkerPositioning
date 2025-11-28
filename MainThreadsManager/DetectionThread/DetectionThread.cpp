// The class DetectionThread manages the main detection process in a separate thread, which can be started and stopped as needed.

#include "DetectionThread.h"
#include <iostream>

// Constructor: starts the main detection thread
DetectionThread::DetectionThread(SettingsReader::MainSettings settings) : Settings(settings), Detection(settings)
{
	detectionThread = std::jthread([this](std::stop_token st) {run(st); });
	running = true;
}

// Destructor: stops the main detection thread
DetectionThread::~DetectionThread()
{
	stopDetectionThread();
}

// Start the main detection process
void DetectionThread::startDetectionThread(FrameData frame)
{
	if (running) {
		std::cout << "[DetectionThread] Thread is already running." << std::endl;
		return;
	}

	currentFrame = frame;

	detectionThread = std::jthread([this](std::stop_token st) {run(st); });

	running = true;
}

// Stop the main detection process
void DetectionThread::stopDetectionThread()
{
	if (!running)
		return;

	detectionThread.request_stop();

	running = false;

}

// Check if the main detection process is running
bool DetectionThread::isRunning()
{
	return running;
}

// 
// --------------------------------
//


// Main detection loop
void DetectionThread::run(std::stop_token st)
{

	std::vector<int> localResults;

	// TODO: hier könnte man ein until... einfügen
	//while (!st.stop_requested())
	//{
	//	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//	std::cout << "DetectionThread running..." << std::endl;

	//	localResults.push_back(rand() % 100); // Dummy result

	//	// preprocess image
	//	cv::Mat preprocessedFrame = preprocessFrame(currentFrame.frame);

	//	// detect apriltag
	//	Pose pose = Detection.detect(preprocessedFrame);

	//	// return pose
	//}

	//std::cout << "DetectionThread running..." << std::endl;

	// preprocess image
	cv::Mat preprocessedFrame = preprocessFrame(currentFrame.frame);

	// detect apriltag
	Pose pose = Detection.detect(preprocessedFrame);

	// return pose

	{
		std::scoped_lock lock(resultMutex);
		results = std::move(pose);
	}

	//std::cout << "DetectionThread stopped." << std::endl;
}

cv::Mat DetectionThread::preprocessFrame(cv::Mat rawFrame)
{
	if (rawFrame.empty()) {
		std::cerr << "[DetectionThread] Error: Could not read raw frame." << std::endl;
		return cv::Mat();
	}

	// define camera matrix and distortion coefficients
	cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) << Settings.fx, 0, Settings.cx, 0, Settings.fy, Settings.cy, 0, 0, 1);
	cv::Mat distCoeffs = (cv::Mat_<double>(1, 5) << Settings.d1, Settings.d2, Settings.d3, Settings.d4, Settings.d5);

	// undistort image based on camera model
	cv::Mat undistorted;
	cv::undistort(rawFrame, undistorted, cameraMatrix, distCoeffs);

	// convert undistorted image to grayscale
	cv::Mat gray;
	cv::cvtColor(undistorted, gray, cv::COLOR_BGR2GRAY);

	return gray;
}