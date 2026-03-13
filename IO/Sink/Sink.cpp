#include "Sink.h"
#include "Logger.h"
#include <chrono>

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/chrono.h>

// Constructor: sink can be configured with config file, where udp send and logging can be enabled / disabled
// unique_ptr is used to manage the lifetime of the udp publisher and result logger, they are only created if enabled in the config
Sink::Sink(const SinkConfig& config) : config_(config)
{
	LOG_TRACE("Initializing Sink...");
	// enable udp send
	if (config_.udpEnabled) {
		LOG_TRACE("Enable UDP sending to {}:{}", config_.udpAddress, config_.udpPort);
		udpPublisher_ = std::make_unique<UdpPublisher>(config_.udpAddress, config_.udpPort);
	}

	// enable logging
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
	
	// notify logging thread in case it's waiting for new events, this will allow it to exit if we are shutting down
	loggingCv_.notify_all();
	LOG_TRACE("Notified logging thread to wake up for shutdown.");

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

// send a pipeline result to both the udp publisher and the logger, if they are enabled in the config
void Sink::send(const PipelineResult& result)
{
	LOG_TRACE("Sending results to sink...");
	// udp
	if (config_.udpEnabled) {
		std::lock_guard<std::mutex> lock(udpMutex_);
		// if we already have a result waiting to be sent, it means we are dropping it, so we increment the drop counter
		if (udpLatest_.has_value()) {
			LogEvent dropEvent;
			dropEvent.type = LogEventType::Drop;
			dropEvent.dropped = *udpLatest_;  // ← WICHTIG
			dropEvent.eventTimestamp = getCurrentTimestamp();
			dropEvent.dropCount = 1;

			{
				std::lock_guard<std::mutex> lock(loggingMutex_);
				loggingQueue_.push(dropEvent);
			}
			LOG_WARN("Dropping result for markerId {} due to UDP send backlog.", result.markerId);
		}
		udpLatest_ = result;
		LOG_TRACE("Result for markerId {} queued for UDP sending.", result.markerId);
	}

	// logging
	if (config_.loggingEnabled) {
		LogEvent event;
		event.type = LogEventType::Result;
		event.result = result;
		event.eventTimestamp = getCurrentTimestamp();

		{
			std::lock_guard<std::mutex> lock(loggingMutex_);
			loggingQueue_.push(event);
		}

		// notify logging thread that we have a new event to process
		loggingCv_.notify_one();
		LOG_TRACE("Result for markerId {} queued for logging.", result.markerId);
	}
}

// worker thread, sends results via udp
void Sink::udpWorkerLoop() {
	LOG_TRACE("UDP worker thread started, entering main loop...");
	while (running_) {
		// get the latest result to send, if any, and reset it
		std::optional<PipelineResult> resultToSend;

		{
			std::lock_guard<std::mutex> lock(udpMutex_);

			if (udpLatest_.has_value()) {
				resultToSend = udpLatest_;
				udpLatest_.reset();
				LOG_TRACE("UDP worker retrieved result for markerId {} to send.", resultToSend->markerId);
			}
		}

		// if we have a result to send, send it via udp
		if (resultToSend.has_value()) {
			try {
				LOG_TRACE("UDP worker sending result...");
				udpPublisher_->send(*resultToSend);
			}
			catch (const std::exception& ex) {
				LOG_ERROR("UDP send failed: {}", ex.what());
				udpErrorCounter_++;

				if (config_.loggingEnabled)
				{
					LogEvent err;
					err.type = LogEventType::SystemError; // or introduce System type
					err.systemMessage = std::string("UDP send failed: ") + ex.what();
					err.eventTimestamp = getCurrentTimestamp();

					std::lock_guard<std::mutex> lock(loggingMutex_);
					loggingQueue_.push(err);
					loggingCv_.notify_one();
					LOG_WARN("Logged UDP send error to logging queue.");
				}
			}
			catch (...)
			{
				LOG_ERROR("UDP send failed with unknown error.");
				udpErrorCounter_++;
			}
		}
		else {
			LOG_TRACE("UDP worker has no result to send, sleeping briefly to avoid busy waiting.");
			// avoid busy waiting
			// TODO: could also be configured via sink config
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

// worker thread, batches log events and writes them to sqlite, also handles logging of udp drops
void Sink::loggingWorkerLoop() {

	LOG_TRACE("Logging worker thread started, entering main loop...");

	// batch configuration from config file
	// batch size determines how many log events we write to sqlite in one transaction, larger batches can improve performance but increase latency
	const size_t BATCH_SIZE = config_.batchSize;
	// batch timeout determines how long we wait for a batch to fill up before writing it to sqlite, this ensures that we don't wait indefinitely if the event rate is low
	const auto BATCH_TIMEOUT = std::chrono::milliseconds(config_.batchTimeoutMs);

	// main loop runs until we are no longer running and there are no more log events to process
	while (running_ || !loggingQueue_.empty()) {

		LOG_TRACE("Logging worker is running and queue has logging objects. Waiting for logging");

		// creates batch vector to hold log events, we reserve space for the batch size to avoid reallocations. not a fixed size, we can have less or more events in batch
		std::vector<LogEvent> batch;
		batch.reserve(BATCH_SIZE);

		// locks mutex, until wait_for condition is not met
		std::unique_lock<std::mutex> lock(loggingMutex_);

		// wait with thread until we have work or shutdown. wait max batch timeout. wakes up and checks condition when notified
		loggingCv_.wait_for(lock, BATCH_TIMEOUT, [&] {
			return !loggingQueue_.empty() || !running_;
			});

		// collect batch
		while (!loggingQueue_.empty() && batch.size() < BATCH_SIZE) {
			LOG_TRACE("Logging worker adding event to batch, current batch size: {}", batch.size());
			batch.push_back(loggingQueue_.front());
			loggingQueue_.pop();
		}

		lock.unlock();

		// if we have no events to log, we can skip the sqlite transaction
		if (batch.empty()) {
			LOG_TRACE("Logging worker woke up but no events to log, going back to waiting.");
			continue;
		}

		// sqlite transaction for batch insert
		// begin transaction, log all events in batch, commit transaction. if any error occurs, we catch it and can decide how to handle it (e.g. retry, log to file, etc.)
		try {
			LOG_TRACE("Logging worker starting SQLite transaction for batch of {} events.", batch.size());
			resultLogger_->beginTransaction();

			for (const auto& event : batch) {
				resultLogger_->logEvent(event);
			}

			resultLogger_->commitTransaction();
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR("SQLite transaction failed: {}", ex.what());
			loggingErrorCounter_++;

			// Attempt to rollback
			LOG_TRACE("Attempting to rollback SQLite transaction after failure...");
			try
			{
				resultLogger_->rollbackTransaction();
			}
			catch (...) {
				LOG_ERROR("SQLite transaction rollback failed.");
			}

			if (!resultLogger_->isHealthy()) {
				LOG_ERROR("ResultLogger is unhealthy after transaction failure, stopping logging worker thread.");
				break;
			}

			LOG_TRACE("SQLite transaction failure handled, logging worker will continue processing future events.");
			// Optional: short sleep to avoid tight failure loop
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		catch (...)
		{
			LOG_ERROR("SQLite transaction failed with unknown error.");
			loggingErrorCounter_++;

			try
			{
				LOG_TRACE("Attempting to rollback SQLite transaction after unknown failure...");
				resultLogger_->rollbackTransaction();
			}
			catch (...) {
				LOG_ERROR("SQLite transaction rollback failed after unknown error.");
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

	}
}

std::string Sink::getCurrentTimestamp() const
{
	auto now = std::chrono::system_clock::now();
	auto formatted = fmt::format(fmt::runtime("{:%FT%TZ}"), now);

	LOG_TRACE("Generating current timestamp for logging, current time is: {}", formatted);
	return fmt::format(fmt::runtime("{:%FT%TZ}"), now);
}