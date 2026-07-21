#pragma once

#include <cstdint>
#include <string>

// Represents a single marker detection result for UDP transmission.
// One UDP message per marker.
struct MarkerMessage
{
	std::string imageTimestamp;  // timestamp of the image

	int32_t markerId = 0;
	int32_t cameraId = 0;
	int32_t markerType = 0;

	int32_t errorCode = 0;  // 0 = success, !=0 = error
	std::string errorMessage = "";  // only relevant for logging

	float posX = 0.0f;
	float posY = 0.0f;
	float posZ = 0.0f;

	float rotX = 0.0f;
	float rotY = 0.0f;
	float rotZ = 0.0f;
};