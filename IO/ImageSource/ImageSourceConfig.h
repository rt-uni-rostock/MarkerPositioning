#pragma once

#include <string>
#include <cstdint>
#include <vector>

#include "SourceModeEnum.h"
#include "CameraSettings.h"

// Configuration object for the ImageAcquisition.

struct ImageSourceConfig {
	SourceMode mode = SourceMode::Live;

	double maxCaptureFPS = 100;

	CameraSettings cameraSettings;
};