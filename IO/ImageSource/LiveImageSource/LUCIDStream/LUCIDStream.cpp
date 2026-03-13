#include "LUCIDStream.h"

#include "ImageSource/ImageFrame.h"

LUCIDStream::LUCIDStream(const ImageSourceConfig& settings) : settings_(settings) {
	//pSystem = Arena::OpenSystem();
}

LUCIDStream::~LUCIDStream() {
	//Arena::CloseSystem(pSystem);
}

bool LUCIDStream::open() {
	// Implement the logic to open the connection to the LUCID camera
	return false;
}

bool LUCIDStream::close() {
	// Implement the logic to close the connection to the LUCID camera
	return false;
}

ImageFrame LUCIDStream::getFrame() {
	// Implement the logic to retrieve the latest frame from the LUCID camera
	// This is a placeholder implementation and should be replaced with actual camera interaction code
	ImageFrame latestData;
	latestData.timestamp = std::chrono::system_clock::now();
	latestData.frameId = frameCounter_++;
	latestData.image = cv::Mat(); // Replace with actual frame data from the camera
	return latestData;
}