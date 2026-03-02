#include "LiveSupervisorMode.h"
#include "DetectionPipeline.h"
#include "ImageSource/IImageSource.h"
#include "Sink.h"
#include "MainSettings.h"
#include "Worker.h"
#include "DetectionResult.h"
#include "Logger.h"
#include <cmath>
#include <iostream>



using steady_clock = std::chrono::steady_clock;

// constructor: initializes the main threads with the given settings
LiveSupervisorMode::LiveSupervisorMode(
	IImageSource& imgSource,
    DetectionPipeline& pipeline,
	Sink& sink,
    const MainSettings& settings
) : imgSource_(imgSource), pipeline_(pipeline), sink_(sink), settings_(settings)
{
	// create workers for the pipeline
    // supervisor is single owner of the workers
    // pushes workers at the end of the vector, creates worker in container
	LOG_TRACE("Initializing LiveSupervisorMode with two workers...");
	workers_.emplace_back(std::make_unique<Worker>(imgSource_, pipeline_));
	workers_.emplace_back(std::make_unique<Worker>(imgSource_, pipeline_));
}

// starts the supervisor thread, which runs the main loop for the live supervisor mode
// also starts the image source, which runs in its own thread and provides frames for the pipeline
void LiveSupervisorMode::start() {
	LOG_TRACE("Starting ImageSource for LiveSupervisorMode...");
	imgSource_.start();
	LOG_TRACE("Starting LiveSupervisorMode supervisor thread...");
	running_ = true;
	supervisorThread_ = std::thread(&LiveSupervisorMode::supervisorLoop, this);
}

// stops the supervisor thread and waits for it to finish
// also stops the image source, which will stop providing frames for the pipeline
void LiveSupervisorMode::stop() {
	LOG_TRACE("Stopping LiveSupervisorMode supervisor thread...");
	running_ = false;
	if (supervisorThread_.joinable()) {
		LOG_TRACE("Joining LiveSupervisorMode supervisor thread...");
		supervisorThread_.join();
	}
	LOG_TRACE("Stopping ImageSource for LiveSupervisorMode...");
	imgSource_.stop();
}

void LiveSupervisorMode::supervisorLoop() {

	LOG_TRACE("LiveSupervisorMode supervisor loop started, calculating interval based on frame rate...");

	// calculate interval based on frame rate
	double interval = round(1000.0 / settings_.frameRate);
	LOG_TRACE("calculated interval: {}", interval);

	// convert interval to milliseconds
	intervalMS_ = std::chrono::milliseconds(static_cast<int>(interval));

	// initialize next stop, where the detection should be finished
	auto nextWakeup = steady_clock::now();

	// main loop
	while (running_) {

		LOG_INFO("LiveSupervisorMode cycle {} starting, next wakeup in {} ms", cycleCount_, intervalMS_.count());

		// update next stop
		nextWakeup += intervalMS_;

		// handle workers and pipeline
		handleCycle();

		// wait until next wakeup time
		std::this_thread::sleep_until(nextWakeup);
	}
}

void LiveSupervisorMode::handleCycle() {

	LOG_TRACE("Handling LiveSupervisorMode cycle {}, acquiring free worker...", cycleCount_);

	// increment cycle count
	++cycleCount_;

	// free worker thread, where the pipeline can be executed
	Worker* worker = acquireFreeWorker();

	// if no worker is available, log an error and skip this cycle
	if (!worker) {
		LOG_ERROR("No free worker available in cycle {}, skipping this cycle.", cycleCount_);

		// TODO: send error via sink
		//sink_.sendError("No free worker in cycle " + std::to_string(cycleCount_));
		return;
	}

	// id of current cycle, used for logging and error handling
	auto cycleId = cycleCount_;

	LOG_TRACE("Starting worker for LiveSupervisorMode cycle {}...", cycleId);
	worker->start(
		cycleId,
		[this, cycleId](const DetectionResult result) {
			LOG_INFO("Worker completed successfully for cycle {}, result: {}", cycleId, result.markerId);
			//sink_sendResult(result);
			// TODO: send result via sink
		},
		[this, cycleId](const std::string& err) {
			LOG_ERROR("Worker failed for cycle {}, error: {}", cycleId, err);
			//sink_sendError("Cycle " + std::to_string(cycleId) + ": " err);
			// TODO: send error via sink
		}
	);

	for (auto& w : workers_) {
		LOG_TRACE("Checking worker status for LiveSupervisorMode cycle {}...", cycleId);
		if (!w->isIdle()) {
			LOG_TRACE("Worker is still busy for cycle {}, checking elapsed time...", cycleId);
			auto elapsed = std::chrono::steady_clock::now() - w->startTime();
			if (elapsed > std::chrono::milliseconds(intervalMS_)) {
				auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
				LOG_ERROR("Worker deadline exceeded for cycle {}, elapsed time: {} ms", cycleId, elapsedMs);
				//sink_sendError("Worker deadline exceeded");
				// TODO: send error via sink
			}
		}
	}
}

// helper method to acquire a free worker, returns nullptr if no worker is available
Worker* LiveSupervisorMode::acquireFreeWorker() {
	// iterate over workers and return the first idle worker
	LOG_TRACE("Acquiring free worker for LiveSupervisorMode cycle {}...", cycleCount_);
	for (auto& w : workers_) {
		if (w->isIdle()) {
			LOG_TRACE("Found free worker for LiveSupervisorMode cycle {}.", cycleCount_);
			return w.get();
		}
	}
	LOG_WARN("No free worker found for LiveSupervisorMode cycle {}.", cycleCount_);
	return nullptr;
}