#pragma once

#include <string>

struct DetectionResult
{
	bool success = false;
	int markerId = -1;

	// TODO: Markerpose hinzufügen
	// TODO: Confidence hinzufügen

	std::string message;
};