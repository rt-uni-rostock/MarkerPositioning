#pragma once
#include "PipelineResult.h"
#include <cstdint>
#include <string>

enum class LogEventType {
	Result = 0,
	Drop = 1,
	SystemError = 2
};

// Logging event used internally by the sink.
// It allows logging of:
// - normal pipeline results (may contain multiple detected markers)
// - drop events (when a camera's previous unsent PipelineResult is overwritten)

struct LogEvent {
	LogEventType type;

	PipelineResult result;  // valid if type == Result (contains list of detected markers)
	PipelineResult dropped; // valid if type == Drop (the overwritten pipeline result)

	uint64_t dropCount = 0; // valid if type == Drop

	std::string systemMessage = ""; // valid if type == SystemError

	std::string eventTimestamp = ""; // when event was generated
};