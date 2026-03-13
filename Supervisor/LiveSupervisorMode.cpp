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

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/chrono.h>

using steady_clock = std::chrono::steady_clock;

// constructor: initializes the main threads with the given settings
LiveSupervisorMode::LiveSupervisorMode(
	IImageSource& imgSource1,
	IImageSource& imgSource2,
    DetectionPipeline& pipeline,
	Sink& sink,
    const MainSettings& settings
) : imgSource1_(imgSource1), imgSource2_(imgSource2), pipeline_(pipeline), sink_(sink), settings_(settings)
{
	// create workers for the pipeline
    // supervisor is single owner of the workers
    // pushes workers at the end of the vector, creates worker in container
	LOG_TRACE("Initializing LiveSupervisorMode with two workers...");
	// TODO: add image source 2 for pipeline
	workersSrc1_.emplace_back(std::make_unique<Worker>(imgSource1_, pipeline_));
	workersSrc1_.emplace_back(std::make_unique<Worker>(imgSource1_, pipeline_));
	workersSrc2_.emplace_back(std::make_unique<Worker>(imgSource2_, pipeline_));
	workersSrc2_.emplace_back(std::make_unique<Worker>(imgSource2_, pipeline_));
}

// starts the supervisor thread, which runs the main loop for the live supervisor mode
// also starts the image source, which runs in its own thread and provides frames for the pipeline
bool LiveSupervisorMode::start() {
	
	LOG_TRACE("Starting ImageSource for LiveSupervisorMode...");
	
	if (!imgSource1_.start()) {
		LOG_ERROR("Failed to start ImageSource 1 for LiveSupervisorMode");
		return false;
	}
	
	if (!imgSource2_.start()) {
		LOG_ERROR("Failed to start ImageSource 2 for LiveSupervisorMode");
		imgSource1_.stop(); // stop the first source if the second fails to start
		return false;
	}

	LOG_TRACE("Starting Sink for LiveSupervisorMode...");
	// TODO start
	if (!sink_.start()) {
		LOG_ERROR("Failed to start Sink for LiveSupervisorMode");
		imgSource1_.stop();
		imgSource2_.stop();
		return false;
	}
	LOG_TRACE("Starting LiveSupervisorMode supervisor thread...");
	running_ = true;
	supervisorThread_ = std::thread(&LiveSupervisorMode::supervisorLoop, this);
	return true;
}

// stops the supervisor thread and waits for it to finish
// also stops the image source, which will stop providing frames for the pipeline
bool LiveSupervisorMode::stop() {
	LOG_TRACE("Stopping LiveSupervisorMode supervisor thread...");
	bool success = true;
	running_ = false;
	if (supervisorThread_.joinable()) {
		LOG_TRACE("Joining LiveSupervisorMode supervisor thread...");
		try {
			supervisorThread_.join();
			LOG_TRACE("LiveSupervisorMode supervisor thread joined successfully.");
		}
		catch (...) {
			LOG_ERROR("Exception occurred while joining LiveSupervisorMode supervisor thread.");
			success = false;
		}
	}

	LOG_TRACE("Stopping ImageSource and Sink for LiveSupervisorMode...");
	success = success && imgSource1_.stop() && imgSource2_.stop() && sink_.stop();

	return success;

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

// handles one cycle of supervisor loop, if interval exceeds time throw error
void LiveSupervisorMode::handleCycle() {

	LOG_TRACE("Handling LiveSupervisorMode cycle {}, acquiring free worker...", cycleCount_);

	// increment cycle count
	++cycleCount_;

	// free worker thread, where the pipeline can be executed
	// TODO: aquire free worker for each image source
	// Worker* workerSrc1 = acquireFreeWorker(imgSrcId(1));
	// Worker* workerSrc2 = acquireFreeWorker(imgSrcId(2));
	Worker* worker = acquireFreeWorker();

	// if no worker is available, log an error and skip this cycle
	if (!worker) {
		LOG_ERROR("No free worker available in cycle {}, skipping this cycle.", cycleCount_);

		// TODO: send error via sink
		//sink_.sendError("No free worker in cycle " + std::to_string(cycleCount_));
		return;
	}
	// TODO: error for each image source if no worker is available
	// if (!workerSrc1) {
	// ...
	// if (!workerSrc2) {
	// ...

	// id of current cycle, used for logging and error handling
	auto cycleId = cycleCount_;

	// start pipeline execution for worker
	LOG_TRACE("Starting worker for LiveSupervisorMode cycle {}...", cycleId);
	worker->start(
		cycleId,
		[this, cycleId, worker](const DetectionResult result) {
			LOG_INFO("Worker completed successfully for cycle {}, result: {}", cycleId, result.markerId);
			
			bool withinDeadline = isWorkerWithinDeadline(worker, cycleId);

			std::string message = "";
			if (!result.success) 
				message += "Detection failed for cycle " + std::to_string(cycleId) + ".";
			if (!withinDeadline) 
				message += " Worker missed deadline for cycle " + std::to_string(cycleId) + ".";

			PipelineResult pipelineResult;
			pipelineResult.imageTimestamp = fmt::format(fmt::runtime("{:%FT%TZ}"), result.timestamp);
			pipelineResult.markerId = result.markerId;
			pipelineResult.cameraId = 1; // TODO: get actual camera id if we have multiple sources
			pipelineResult.markerType = 0;
			pipelineResult.errorCode = result.success ? 0 : 1;
			pipelineResult.errorMessage = result.success ? "" : "Detection failed";
			pipelineResult.posX = result.pose.x;
			pipelineResult.posY = result.pose.y;
			pipelineResult.posZ = result.pose.z;
			pipelineResult.rotX = result.pose.roll;
			pipelineResult.rotY = result.pose.pitch;
			pipelineResult.rotZ = result.pose.yaw;
			
			// send pipeline result to sink
			sink_.send(pipelineResult);
		},
		[this, cycleId](const std::string& err) {
			LOG_ERROR("Worker failed for cycle {}, error: {}", cycleId, err);

			PipelineResult pipelineResult;
			pipelineResult.imageTimestamp = fmt::format(fmt::runtime("{:%FT%TZ}"), std::chrono::system_clock::now());
			pipelineResult.errorCode = 1;
			pipelineResult.errorMessage = err;
			sink_.send(pipelineResult);
		}
	);
}

// helper method to acquire a free worker, returns nullptr if no worker is available
Worker* LiveSupervisorMode::acquireFreeWorker() {
	// iterate over workers and return the first idle worker
	LOG_TRACE("Acquiring free worker for LiveSupervisorMode cycle {}...", cycleCount_);
	for (auto& w : workersSrc1_) {
		if (w->isIdle()) {
			LOG_TRACE("Found free worker for LiveSupervisorMode cycle {}.", cycleCount_);
			return w.get();
		}
	}
	LOG_WARN("No free worker found for LiveSupervisorMode cycle {}.", cycleCount_);
	return nullptr;
}

// after successful worker execution, check whether the worker is within the deadline
bool LiveSupervisorMode::isWorkerWithinDeadline(Worker* worker, uint64_t cycleId) {
	LOG_TRACE("Checking worker execution time for LiveSupervisorMode cycle {} ...", cycleId);
	auto elapsed = std::chrono::steady_clock::now() - worker->startTime();
	if (elapsed > std::chrono::milliseconds(intervalMS_)) {
		auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
		LOG_ERROR("Worker deadline exceeded for cycle {}, elapsed time: {} ms", cycleCount_, elapsedMs);
		return false;
	}
	LOG_TRACE("Worker completed within deadline for LiveSupervisorMode cycle {}, elapsed time: {} ms", cycleId, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
	return true;
}