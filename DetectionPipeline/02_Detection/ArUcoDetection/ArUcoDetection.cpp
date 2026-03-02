#include "ArUcoDetection.h"
#include "ImageSource/ImageFrame.h"
#include "DetectionResult.h"
#include "Logger.h"

ArUcoDetection::ArUcoDetection(const DetectionPipelineConfig& config) {
	// Constructor implementation
	LOG_TRACE("ArUcoDetection created with provided config. Currently not implemented.");
}

ArUcoDetection::~ArUcoDetection() {
	// Destructor implementation
	LOG_TRACE("ArUcoDetection is being destroyed. Currently not implemented.");

}

DetectionResult ArUcoDetection::process(const ImageFrame& frame) {
	LOG_TRACE("Processing frame with ID: {} in ArUcoDetection. Currently not implemented.", frame.frameId);
	// Detection implementation
	DetectionResult result;
	result.frameId = frame.frameId;
	result.timestamp = frame.timestamp;
	result.markerId = -1; // Set to -1 if no marker is detected
	result.message = "not implemented yet";
	result.pose = Pose{ 0,0,0,0,0,0 }; // Default pose if no marker is detected
	result.success = false; // Set to true if detection is successful
	return result;
}