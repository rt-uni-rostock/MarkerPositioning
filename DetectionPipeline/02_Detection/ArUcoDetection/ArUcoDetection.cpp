#include "ArUcoDetection.h"
#include "ImageSource/ImageFrame.h"
#include "DetectionResult.h"

ArUcoDetection::ArUcoDetection(const DetectionPipelineConfig& config) {
	// Constructor implementation
}

ArUcoDetection::~ArUcoDetection() {
	// Destructor implementation
}

DetectionResult ArUcoDetection::process(const ImageFrame& frame) {
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