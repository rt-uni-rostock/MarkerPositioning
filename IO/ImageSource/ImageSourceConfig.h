#pragma once

#include <string>
#include <cstdint>
#include <vector>

#include "SourceModeEnum.h"
#include "StreamTypeEnum.h"

// Configuration object for the ImageAcquisition.

struct ImageSourceConfig {
	SourceMode mode = SourceMode::Live;
	StreamType type = StreamType::RTP;

	std::string srcUrl; // for stream
	std::string filePath; // for recorded mode
	int streamId = 0;

	double maxCaptureFPS = 100;
};