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

	std::string rtspUrl; // for RTSP stream
	std::string filePath; // for recorded mode
};