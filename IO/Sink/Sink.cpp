#include "Sink.h"
#include "MarkerMessage.h"
#include "Logger.h"
#include <chrono>
#include <ctime>

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/chrono.h>

// Constructor: sink can be configured with config file, where udp send and logging can be enabled / disabled
// unique_ptr is used to manage the lifetime of the udp publisher and result logger, they are only created if enabled in the config
Sink::Sink(const SinkConfig& config) : config_(config)
{
	LOG_TRACE("Initializing Sink...");
	if (config_.udpEnabled) {
		LOG_TRACE("Enable UDP sending to {}:{}", config_.udpAddress, config_.udpPort);
		udpPublisher_ = std::make_unique<UdpPublisher>(config_.udpAddress, config_.udpPort);
	}

	if (config_.loggingEnabled) {
		LOG_TRACE("Enable logging to SQLite database at {}", config_.sqliteFilePath);
		resultLogger_ = std::make_unique<ResultLogger>(config_.sqliteFilePath);
	}
}

// Destructor: ensure threads are stopped and joined before destruction to avoid undefined behavior
Sink::~Sink()
{
	LOG_TRACE("Destroying Sink, stopping worker threads...");
	stop();
}

// start the worker threads for udp sending and logging, they will run until stop() is called
bool Sink::start()
{
	LOG_TRACE("Starting Sink worker threads...");
	running_ = true;
	if (config_.udpEnabled) {
		try {
			udpThread_ = std::thread(&Sink::udpWorkerLoop, this);
			LOG_TRACE("UDP worker thread started.");
		}
		catch (...) {
			LOG_ERROR("Failed to start UDP worker thread.");
			return false;
		}
	}
	if (config_.loggingEnabled) {
		try {
			loggingThread_ = std::thread(&Sink::loggingWorkerLoop, this);
			LOG_TRACE("Logging worker thread started.");
		}
		catch (...) {
			LOG_ERROR("Failed to start logging worker thread.");
			return false;
		}
	}
	return true;
}

// stop worker threads, notify them to wake up if they are waiting, and join them to ensure clean shutdown
bool Sink::stop()
{
	LOG_TRACE("Stopping Sink worker threads...");
	running_ = false;

	bool success = true;

	udpCv_.notify_all();
	loggingCv_.notify_all();
	LOG_TRACE("Notified worker threads to wake up for shutdown.");

	if (udpThread_.joinable()) {
		try {
			LOG_TRACE("Joining UDP worker thread...");
			udpThread_.join();
			LOG_TRACE("UDP worker thread joined successfully.");
		}
		catch (...) {
			LOG_ERROR("Unknown exception occurred while joining UDP worker thread.");
			success = false;
		}
	}

	if (loggingThread_.joinable()) {
		loggingCv_.notify_all();
		try {
			LOG_TRACE("Joining logging worker thread...");
			loggingThread_.join();
			LOG_TRACE("Logging worker thread joined successfully.");
		}
		catch (...) {
			LOG_ERROR("Unknown exception occurred while joining logging worker thread.");
			success = false;
		}
	}

	return success;
}

// send a pipeline result for a specific camera: overwrite latest-only slot (UDP),
// enqueue full history entry (logging). Non-blocking for the calling (worker) thread.
void Sink::send(uint8_t cameraId, const PipelineResult& result)
{
	LOG_TRACE("Sending pipeline result for camera {} to sink...", cameraId);

	// udp: latest-only per camera
	if (config_.udpEnabled) {
		std::lock_guard<std::mutex> lock(udpMutex_);

		auto it = udpLatestPerCamera_.find(cameraId);
		if (it != udpLatestPerCamera_.end()) {
			// previous result for this camera was not sent yet -> drop it, log the drop
			if (config_.loggingEnabled) {
				LogEvent dropEvent;
				dropEvent.type = LogEventType::Drop;
				dropEvent.dropped = it->second;
				dropEvent.dropCount = 1;
				dropEvent.eventTimestamp = getCurrentTimestamp();

				std::lock_guard<std::mutex> logLock(loggingMutex_);
				loggingQueue_.push(dropEvent);
				loggingCv_.notify_one();
			}
			LOG_WARN("Dropping previous unsent UDP result for camera {} (backlog).", cameraId);
		}

		udpLatestPerCamera_[cameraId] = result;
		udpCv_.notify_one();
		LOG_TRACE("Result for camera {} queued for UDP sending.", cameraId);
	}

	// logging: always recorded, never dropped
	if (config_.loggingEnabled) {
		LogEvent event;
		event.type = LogEventType::Result;
		event.result = result;
		event.eventTimestamp = getCurrentTimestamp();

		{
			std::lock_guard<std::mutex> lock(loggingMutex_);
			loggingQueue_.push(event);
		}

		loggingCv_.notify_one();
		LOG_TRACE("Result for camera {} with {} markers queued for logging.", cameraId, result.detectedMarkers.size());
	}
}

// single shared worker thread: sequentially iterates all cameras with pending
// results, and for each camera all detected markers, sending one UDP message
// per marker. This is fast enough in practice (a few microseconds per sendto())
// to finish well before the next pipeline cycle produces new results.
void Sink::udpWorkerLoop() {
	LOG_TRACE("UDP worker thread started, entering main loop...");
	while (running_) {
		std::unordered_map<uint8_t, PipelineResult> resultsToSend;

		{
			std::unique_lock<std::mutex> lock(udpMutex_);
			udpCv_.wait_for(lock, std::chrono::milliseconds(10), [&] {
				return !udpLatestPerCamera_.empty() || !running_;
				});

			// take a snapshot of all pending camera results, then clear the map
			resultsToSend.swap(udpLatestPerCamera_);
		}

		// sequential loop: camera by camera, marker by marker
		for (const auto& [cameraId, result] : resultsToSend) {
			for (const auto& pose : result.detectedMarkers) {
				MarkerMessage msg;
				msg.imageTimestamp = result.imageTimestamp;
				msg.markerId = pose.tagId;
				msg.cameraId = result.cameraId;
				msg.markerType = result.markerType;
				msg.errorCode = result.errorCode;
				msg.errorMessage = result.errorMessage;
				msg.posX = pose.x;
				msg.posY = pose.y;
				msg.posZ = pose.z;
				msg.rotX = pose.roll;
				msg.rotY = pose.pitch;
				msg.rotZ = pose.yaw;

				try {
					LOG_TRACE("UDP worker sending marker message for camera {}, markerId {}...", cameraId, msg.markerId);
					udpPublisher_->send(msg);
				}
				catch (const std::exception& ex) {
					LOG_ERROR("UDP send failed for camera {}: {}", cameraId, ex.what());
					udpErrorCounter_++;

					if (config_.loggingEnabled)
					{
						LogEvent err;
						err.type = LogEventType::SystemError;
						err.systemMessage = fmt::format("UDP send failed for camera {}: {}", cameraId, ex.what());
						err.eventTimestamp = getCurrentTimestamp();

						std::lock_guard<std::mutex> lock(loggingMutex_);
						loggingQueue_.push(err);
						loggingCv_.notify_one();
						LOG_WARN("Logged UDP send error to logging queue.");
					}
				}
				catch (...)
				{
					LOG_ERROR("UDP send failed for camera {} with unknown error.", cameraId);
					udpErrorCounter_++;
				}
			}
		}
	}
	LOG_TRACE("UDP worker thread exiting.");
}

// worker thread, batches log events and writes them to sqlite
void Sink::loggingWorkerLoop() {
	LOG_TRACE("Logging worker thread started, entering main loop...");
	while (running_) {
		std::vector<LogEvent> batch;

		{
			std::unique_lock<std::mutex> lock(loggingMutex_);
			loggingCv_.wait_for(lock, std::chrono::milliseconds(config_.batchTimeoutMs), [&] {
				return loggingQueue_.size() >= config_.batchSize || !running_;
				});

			while (!loggingQueue_.empty() && batch.size() < config_.batchSize) {
				batch.push_back(std::move(loggingQueue_.front()));
				loggingQueue_.pop();
			}
		}

		if (!batch.empty() && resultLogger_) {
			try {
				resultLogger_->beginTransaction();
				for (const auto& event : batch) {
					resultLogger_->logEvent(event);
				}
				resultLogger_->commitTransaction();
			}
			catch (const std::exception& ex) {
				LOG_ERROR("Logging batch failed: {}", ex.what());
				loggingErrorCounter_++;
				try {
					resultLogger_->rollbackTransaction();
				}
				catch (...) {
					LOG_ERROR("Rollback after failed logging batch also failed.");
				}
			}
		}
	}

	// flush remaining events on shutdown
	if (resultLogger_) {
		std::vector<LogEvent> remaining;
		{
			std::lock_guard<std::mutex> lock(loggingMutex_);
			while (!loggingQueue_.empty()) {
				remaining.push_back(std::move(loggingQueue_.front()));
				loggingQueue_.pop();
			}
		}
		if (!remaining.empty()) {
			try {
				resultLogger_->beginTransaction();
			 for (const auto& event : remaining) {
					resultLogger_->logEvent(event);
				}
				resultLogger_->commitTransaction();
			}
			catch (const std::exception& ex) {
				LOG_ERROR("Final logging flush failed: {}", ex.what());
			}
		}
	}

	LOG_TRACE("Logging worker thread exiting.");
}

// returns current timestamp as ISO 8601 string
std::string Sink::getCurrentTimestamp() const
{
	return fmt::format(fmt::runtime("{:%FT%TZ}"), std::chrono::system_clock::now());
}