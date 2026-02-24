#pragma once
#include <string>
#include <cstdint>

// Configuration object for the Sink.

struct SinkConfig {
	bool udpEnabled = true;
	bool loggingEnabled = true;

	std::string udpAddress = "127.0.0.1";
	uint16_t udpPort = 5000;

	std::string sqliteFilePath = "result.db";

	// batch configuration
	size_t batchSize = 10;
	int batchTimeoutMs = 500;
};