#include "ArUcoDetection.h"
#include "ImageSource/ImageFrame.h"
#include "DetectionResult.h"
#include "Logger.h"

ArUcoDetection::ArUcoDetection(const DetectionPipelineConfig& config) {
	LOG_TRACE("ArUcoDetection created with provided config. Currently not implemented.");
}

ArUcoDetection::~ArUcoDetection() {
	LOG_TRACE("ArUcoDetection is being destroyed. Currently not implemented.");
}

DetectionResult ArUcoDetection::process(const ImageFrame& frame) {
	LOG_TRACE("Processing frame with ID: {} in ArUcoDetection. Currently not implemented.", frame.frameId);
	
	DetectionResult result;
	result.frameId = frame.frameId;
	result.timestamp = frame.timestamp;
	result.message = "not implemented yet";
	result.success = false;
	
	// Füge eine leere Pose mit tagId = -1 hinzu
	Pose emptyPose{ 0, 0, 0, 0, 0, 0, -1, 0 };
	result.detectedMarkers.push_back(emptyPose);
	
	return result;
}