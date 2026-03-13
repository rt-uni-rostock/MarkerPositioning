#pragma once
#include "SinkConfig.h"
#include "UdpPublisher.h"
#include "ResultLogger.h"
#include "LogEvent.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <optional>
#include <atomic>

// Sink orchestrates:
//  - UDP sending (latest-only)
//  - Logging (full history + batching)
// uses two seperate worker threads.

class Sink
{
public:
	// Constructor and destructor
	explicit Sink(const SinkConfig& config);
	~Sink();

	// start and stop the worker threads, they will run until stop() is called
	bool start();
	bool stop();

	// main send method, can be called by supervisor to send pipeline results to sink
	void send(const PipelineResult& result);

private:
	// worker thread methods
	void udpWorkerLoop();
	void loggingWorkerLoop();

	std::string getCurrentTimestamp() const;

	// configuration object, contains settings for udp and logging
	SinkConfig config_;

	// atomic flag to control the running state of the worker threads
	std::atomic<bool> running_{ false };

	// udp path
	std::thread udpThread_;
	std::mutex udpMutex_;
	std::optional<PipelineResult> udpLatest_;
	std::unique_ptr<UdpPublisher> udpPublisher_;
	std::atomic<uint64_t> udpErrorCounter_{ 0 };

	// logging path
	std::thread loggingThread_;
	std::mutex loggingMutex_;
	std::condition_variable loggingCv_;
	std::queue<LogEvent> loggingQueue_;
	std::unique_ptr<ResultLogger> resultLogger_;
	std::atomic<uint64_t> loggingErrorCounter_{ 0 };
};