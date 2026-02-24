#pragma once
#include <cstdint>
#include <string>

// Represents the result of one pipeline execution.
// This object is passed from supervisor to sink

struct PipelineResult {
	std::string imageTimestamp; // timestamp of the image

	int32_t markerId = 0;
	int32_t cameraId = 0;
	int32_t markerType = 0;

	int32_t errorCode = 0; // 0 = success, !=0 = error
	std::string errorMessage = ""; // only relevant for logging

	float posX = 0.0;
	float posY = 0.0;
	float posZ = 0.0;

	float rotX = 0.0;
	float rotY = 0.0;
	float rotZ = 0.0;
};