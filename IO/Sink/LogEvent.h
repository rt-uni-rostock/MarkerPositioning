#pragma once
#include "PipelineResult.h"
#include <cstdint>

enum class LogEventType {
	Result = 0,
	Drop = 1,
	SystemError = 2
};

// Logging event used internally by the sink.
// It allows logging of:
// - normal pipeline results
// - drop events (when udp overrides pending result)

struct LogEvent {
	LogEventType type;

	PipelineResult result; // valid if type == Result
	PipelineResult dropped; // valid if type == Drop

	uint64_t dropCount = 0; // valid if type == Drop

	std::string systemMessage = ""; // valid if type == SystemError

	std::string eventTimestamp = ""; // when event was generated
};