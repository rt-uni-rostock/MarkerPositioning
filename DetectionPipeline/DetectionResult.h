#pragma once

#include <string>
#include <chrono>
#include <vector>
#include "02_Detection/Pose.h"

struct DetectionResult
{
	bool success = false;
	int frameId = -1;
	std::chrono::system_clock::time_point timestamp;

	// Unterstützung für mehrere Marker pro Frame
	std::vector<Pose> detectedMarkers;

	std::string message;
};