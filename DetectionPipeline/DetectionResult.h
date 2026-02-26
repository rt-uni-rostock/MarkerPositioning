#pragma once

#include <string>
#include <chrono>
#include "02_Detection/Pose.h"

struct DetectionResult
{
	bool success = false;
	int markerId = -1;
	int frameId = -1;
	std::chrono::system_clock::time_point timestamp;

	Pose pose{ 0,0,0,0,0,0 };

	// TODO: Markerpose hinzufügen
	// TODO: Confidence hinzufügen

	std::string message;
};