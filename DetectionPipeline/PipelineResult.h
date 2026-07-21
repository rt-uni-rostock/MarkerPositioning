#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "02_Detection/Pose.h"

// Represents the result of one pipeline execution.
// This object is passed from supervisor to sink

struct PipelineResult {
	std::string imageTimestamp; // timestamp of the image

	int32_t cameraId = 0;
	int32_t markerType = 0;

	int32_t errorCode = 0; // 0 = success, !=0 = error
	std::string errorMessage = ""; // only relevant for logging

	// Liste von erkannten Markern (Posen)
	std::vector<Pose> detectedMarkers;
};