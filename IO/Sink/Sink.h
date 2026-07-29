#pragma once
#include "SinkConfig.h"
#include "UdpPublisher.h"
#include "ResultLogger.h"
#include "LogEvent.h"
#include "PipelineResult.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <cstdint>

// Sink orchestrates:
//  - UDP sending: single shared thread + single socket, latest-only per camera
//    (no queueing). If a new result arrives for a camera whose previous result
//    was not sent yet, the old one is overwritten and a Drop event is logged.
//    Sending is sequential (camera by camera, marker by marker) - this is fast
//    enough (a few microseconds per sendto()) to reliably finish before the
//    next pipeline cycle in practice.
//  - Logging: full history, batched writes, separate thread.

class Sink
{
public:
	explicit Sink(const SinkConfig& config);
	~Sink();

	bool start();
	bool stop();

	// send a pipeline result for a specific camera to the sink (UDP latest-only + logging).
	void send(uint8_t cameraId, const PipelineResult& result);

private:
	void udpWorkerLoop();
	void loggingWorkerLoop();

	std::string getCurrentTimestamp() const;

	SinkConfig config_;
	std::atomic<bool> running_{ false };

	// udp path: single thread, single socket, latest-only per camera
	std::thread udpThread_;
	std::mutex udpMutex_;
	std::condition_variable udpCv_;
	std::unordered_map<uint8_t, PipelineResult> udpLatestPerCamera_;
	std::unique_ptr<UdpPublisher> udpPublisher_;
	std::atomic<uint64_t> udpErrorCounter_{ 0 };

	// logging path: shared queue, separate thread
	std::thread loggingThread_;
	std::mutex loggingMutex_;
	std::condition_variable loggingCv_;
	std::queue<LogEvent> loggingQueue_;
	std::unique_ptr<ResultLogger> resultLogger_;
	std::atomic<uint64_t> loggingErrorCounter_{ 0 };
};