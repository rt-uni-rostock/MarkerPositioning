#include "Worker.h"
#include "ImageSource/IImageSource.h"
#include "DetectionPipeline.h"
#include "DetectionResult.h"

#include "Logger.h"

Worker::Worker(IImageSource& source, DetectionPipeline& pipeline, uint8_t cameraId)
	: source_(source), pipeline_(pipeline), cameraId_(cameraId) {
	LOG_TRACE("Worker created.");
}

Worker::~Worker() {
	if (thread_.joinable()) {
		thread_.join();
	}
	LOG_TRACE("Worker destroyed.");
}

// check if worker is idle
bool Worker::isIdle() const {
	return state_.load() == WorkerState::Idle;
}

// start worker thread for pipeline execution
void Worker::start(uint64_t cycleId,
	std::function<void(const DetectionResult&)> onSuccess,
	std::function<void(std::string)> onError) {

	LOG_TRACE("Starting worker for cycle {}...", cycleId);

	if (!isIdle()) {
		LOG_ERROR("Cannot start worker for cycle {}, worker is already running.", cycleId);
		onError("Worker is already running");
		return;
	}

	// set state to running and store start time
	state_ = WorkerState::Running;

	startTime_ = std::chrono::steady_clock::now();

	LOG_TRACE("Starting thread for worker cycle {}...", cycleId);

	// start thread for pipeline execution
	thread_ = std::thread([=, this]() {
		try {

			// live image acquisition
			LOG_TRACE("Worker cycle {}: acquiring latest frame from image source...", cycleId);
			ImageFrame latestFrame = source_.getLatestFrame();

			// TODO: validation check, error handling

			// detection pipeline
			LOG_TRACE("Worker cycle {}: processing latest frame through detection pipeline...", cycleId);
			DetectionResult result = pipeline_.process(latestFrame);

			// TODO: validation check, error handling

			// return result via onSuccess callback
			LOG_TRACE("Worker cycle {}: pipeline processing completed, returning result via onSuccess callback...", cycleId);
			onSuccess(result);
		}
		catch (const std::exception& e) {
			// return error via onError callback
			LOG_ERROR("Worker cycle {}: exception occurred during execution: {}", cycleId, e.what());
			onError(e.what());
		}

		// set state back to idle after execution
		state_ = WorkerState::Idle;
		});

	// detach thread to allow independent execution
	LOG_TRACE("Worker cycle {}: detaching thread for independent execution...", cycleId);
	thread_.detach();
}

// get start time of current execution
std::chrono::steady_clock::time_point Worker::startTime() const {
	return startTime_;
}