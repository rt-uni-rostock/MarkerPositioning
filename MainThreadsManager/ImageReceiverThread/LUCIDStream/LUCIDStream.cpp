#include "LUCIDStream.h"

LUCIDStream::LUCIDStream(SettingsReader::MainSettings settings) : Settings(settings) {
	//pSystem = Arena::OpenSystem();
}

LUCIDStream::~LUCIDStream() {
	//Arena::CloseSystem(pSystem);
}

FrameData LUCIDStream::getFrame() {
	// Implement the logic to retrieve the latest frame from the LUCID camera
	// This is a placeholder implementation and should be replaced with actual camera interaction code
	FrameData latestData;
	latestData.timestamp = std::chrono::system_clock::now();
	latestData.frame = cv::Mat(); // Replace with actual frame data from the camera
	return latestData;
}