#include "LiveSupervisorMode.h"
#include "DetectionPipeline.h"
#include "ImageSource/IImageSource.h"
#include "Sink.h"
#include "Worker.h"
#include "PipelineResult.h"
#include "Logger.h"
#include "GeneralSettings.h"
#include <cmath>
#include <iostream>

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/fmt/chrono.h>

using steady_clock = std::chrono::steady_clock;

// constructor: initializes the main threads with the given settings
LiveSupervisorMode::LiveSupervisorMode(
	const std::vector<IImageSource*>& imgSources,
	const std::vector<DetectionPipeline*>& pipelines,
	Sink& sink,
    const GeneralSettings& settings
) : imgSources_(imgSources), pipelines_(pipelines), sink_(sink), settings_(settings)
{
	// create workers for the pipeline
    // supervisor is single owner of the workers
    // pushes workers at the end of the vector, creates worker in container
	LOG_TRACE("Initializing LiveSupervisorMode with two workers for each camera...");

	// foreach active camera create two workers
	size_t idx = 0;
	for (const auto& cam : settings_.cameras) {
		if (cam.active) {
			LOG_TRACE("Creating workers for camera {}...", cam.id);

			// collect all pipelines for this camera (expect two — one per worker)
			std::vector<DetectionPipeline*> cameraPipelines;
			for (const auto& p : pipelines_) {
				if (p->getCameraId() == cam.id) {
					cameraPipelines.push_back(p);
				}
			}

			if (cameraPipelines.size() >= 2) {
				workers_.emplace_back(std::make_unique<Worker>(*imgSources_[idx], *cameraPipelines[0], cam.id));
				workers_.emplace_back(std::make_unique<Worker>(*imgSources_[idx], *cameraPipelines[1], cam.id));
			} else if (cameraPipelines.size() == 1) {
				LOG_WARN("Only one pipeline found for camera {}, both workers will share it.", cam.id);
				workers_.emplace_back(std::make_unique<Worker>(*imgSources_[idx], *cameraPipelines[0], cam.id));
				workers_.emplace_back(std::make_unique<Worker>(*imgSources_[idx], *cameraPipelines[0], cam.id));
			} else {
				LOG_ERROR("No pipeline found for camera ID {}", cam.id);
			}

			idx++;
		}
	}
}

// starts the supervisor thread, which runs the main loop for the live supervisor mode
// also starts the image source, which runs in its own thread and provides frames for the pipeline
bool LiveSupervisorMode::start() {
	
	LOG_TRACE("Starting all ImageSources for LiveSupervisorMode...");

	// start all image sources
	for (const auto& imgSource : imgSources_) {
		if (!imgSource->start()) {
			LOG_ERROR("Failed to start an ImageSource for LiveSupervisorMode");
			// stop all previously started sources
			for (const auto& startedSource : imgSources_) {
				startedSource->stop();
			}
			return false;
		}
	}

	LOG_TRACE("Starting Sink for LiveSupervisorMode...");
	// TODO start
	if (!sink_.start()) {
		LOG_ERROR("Failed to start Sink for LiveSupervisorMode");
		
		// stop all image sources
		for (const auto& imgSource : imgSources_) {
			imgSource->stop();
		}

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

	LOG_TRACE("Joining all workers for LiveSupervisorMode...");
	for (auto& worker : workers_) {
		worker->join();
	}

	LOG_TRACE("Stopping all ImageSources for LiveSupervisorMode...");
	for (const auto& imgSource : imgSources_) {
		if (!imgSource->stop()) {
			LOG_ERROR("Failed to stop an ImageSource for LiveSupervisorMode");
			success = false;
		}
	}

	LOG_TRACE("Stopping Sink for LiveSupervisorMode...");
	success = success && sink_.stop();

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

	LOG_TRACE("Handling LiveSupervisorMode cycle {}, acquiring free workers...", cycleCount_);

	// increment cycle count
	++cycleCount_;

	// id of current cycle, used for logging and error handling
	auto cycleId = cycleCount_;

	// iterate over all configured cameras
	for (const auto& cam : settings_.cameras) {
		// skip inactive cameras
		if (!cam.active) {
			continue;
		}

		// acquire free worker for this specific camera
		Worker* worker = acquireFreeWorker(cam.id);

		// if no worker is available for this camera, log an error and skip
		if (!worker) {
			LOG_ERROR("No free worker available for camera {} in cycle {}, skipping this image source.", cam.id, cycleId);
			// Optional: send error via sink
			// PipelineResult errResult;
			// errResult.cameraId = cam.id;
			// errResult.errorCode = 1;
			// errResult.errorMessage = "No free worker in cycle " + std::to_string(cycleId);
			// sink_.send(errResult);
			continue;
		}

		// start pipeline execution for worker
		LOG_TRACE("Starting worker for camera {} in LiveSupervisorMode cycle {}...", cam.id, cycleId);
		
		worker->start(
			cycleId,
			[this, cycleId, worker, cameraId = cam.id](const PipelineResult& result) {
				LOG_INFO("Worker completed successfully for camera {} in cycle {}, marker count: {}",
					cameraId, cycleId, result.detectedMarkers.size());

				bool withinDeadline = isWorkerWithinDeadline(worker, cycleId);

				std::string message = "";
				if (result.errorCode != 0)
					message += "Detection failed for cycle " + std::to_string(cycleId) + ".";
				if (!withinDeadline)
					message += " Worker missed deadline for cycle " + std::to_string(cycleId) + ".";

				// send pipeline result to sink: latest-only UDP fan-out per marker + full logging
				sink_.send(cameraId, result);
			},
			[this, cycleId, cameraId = cam.id](const std::string& err) {
				LOG_ERROR("Worker failed for camera {} in cycle {}, error: {}", cameraId, cycleId, err);

				PipelineResult pipelineResult;
				pipelineResult.imageTimestamp = fmt::format(fmt::runtime("{:%FT%TZ}"), std::chrono::system_clock::now());
				pipelineResult.cameraId = cameraId;
				pipelineResult.errorCode = 1;
				pipelineResult.errorMessage = err;
				sink_.send(cameraId, pipelineResult);
			}
		);
	}
}

// helper method to acquire a free worker, returns nullptr if no worker is available
Worker* LiveSupervisorMode::acquireFreeWorker(uint8_t cameraId) {
	// iterate over workers and return the first idle worker belonging to the specified camera
	LOG_TRACE("Acquiring free worker for LiveSupervisorMode cycle {} and camera {} ...", cycleCount_, cameraId);
	for (auto& w : workers_) {
		if (w->getCameraId() == cameraId && w->isIdle()) {
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