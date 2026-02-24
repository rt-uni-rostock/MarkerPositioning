#include "Worker.h"
#include "IImageSource.h"
#include "DetectionPipeline.h"
#include "DetectionResult.h"

Worker::Worker(IImageSource& source, DetectionPipeline& pipeline)
	: source_(source), pipeline_(pipeline) {
}

Worker::~Worker() {
	if (thread_.joinable()) {
		thread_.join();
	}
}

// check if worker is idle
bool Worker::isIdle() const {
	return state_.load() == WorkerState::Idle;
}

// start worker thread for pipeline execution
void Worker::start(uint64_t cycleId,
	std::function<void(const DetectionResult&)> onSuccess,
	std::function<void(std::string)> onError) {

	if (!isIdle()) {
		onError("Worker is already running");
		return;
	}

	// set state to running and store start time
	state_ = WorkerState::Running;

	startTime_ = std::chrono::steady_clock::now();

	// start thread for pipeline execution
	thread_ = std::thread([=, this]() {
		try {
			// live image acquisition
			ImageFrame latestFrame = source_.getLatestFrame();

			// detection pipeline
			DetectionResult result = pipeline_.process(latestFrame);

			// return result via onSuccess callback
			onSuccess(result);
		}
		catch (const std::exception& e) {
			// return error via onError callback
			onError(e.what());
		}

		// set state back to idle after execution
		state_ = WorkerState::Idle;
		});

	// detach thread to allow independent execution
	thread_.detach();
}

// get start time of current execution
std::chrono::steady_clock::time_point Worker::startTime() const {
	return startTime_;
}