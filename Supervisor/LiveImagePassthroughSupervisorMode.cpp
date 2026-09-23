#include "LiveImagePassthroughSupervisorMode.h"

#include <cmath>
#include <exception>

#include "ImageSource/IImageSource.h"
#include "ImageUdpPublisher.h"
#include "GeneralSettings.h"
#include "Logger.h"

using steady_clock = std::chrono::steady_clock;

LiveImagePassthroughSupervisorMode::LiveImagePassthroughSupervisorMode(
	const std::vector<IImageSource*>& imgSources,
	const std::vector<uint8_t>& cameraIds,
	ImageUdpPublisher& imagePublisher,
	const GeneralSettings& settings
) : imgSources_(imgSources), cameraIds_(cameraIds), imagePublisher_(imagePublisher), settings_(settings) {
}

bool LiveImagePassthroughSupervisorMode::start() {
	if (settings_.frameRate <= 0.0) {
		LOG_ERROR("Invalid frameRate {} for LiveImagePassthroughSupervisorMode.", settings_.frameRate);
		return false;
	}

	for (const auto& imgSource : imgSources_) {
		if (!imgSource->start()) {
			for (const auto& startedSource : imgSources_) {
				startedSource->stop();
			}
			return false;
		}
	}

	// Phase 2: begin capture on all sources only after every one of them has
	// been opened (see LiveSupervisorMode::start() for why this matters for
	// LUCID GigE cameras).
	for (const auto& imgSource : imgSources_) {
		if (!imgSource->beginCapture()) {
			for (const auto& startedSource : imgSources_) {
				startedSource->stop();
			}
			return false;
		}
	}

	const double interval = std::round(1000.0 / settings_.frameRate);
	intervalMs_ = std::chrono::milliseconds(static_cast<int>(interval));

	running_ = true;
	supervisorThread_ = std::thread(&LiveImagePassthroughSupervisorMode::supervisorLoop, this);
	return true;
}

bool LiveImagePassthroughSupervisorMode::stop() {
	bool success = true;
	running_ = false;

	if (supervisorThread_.joinable()) {
		try {
			supervisorThread_.join();
		}
		catch (const std::exception& ex) {
			LOG_ERROR("Failed to join LiveImagePassthrough supervisor thread: {}", ex.what());
			success = false;
		}
	}

	for (const auto& imgSource : imgSources_) {
		if (!imgSource->stop()) {
			success = false;
		}
	}

	return success;
}

void LiveImagePassthroughSupervisorMode::supervisorLoop() {
	auto nextWakeup = steady_clock::now();

	while (running_) {
		nextWakeup += intervalMs_;
		handleCycle();
		std::this_thread::sleep_until(nextWakeup);
	}
}

void LiveImagePassthroughSupervisorMode::handleCycle() {
	for (size_t idx = 0; idx < imgSources_.size() && idx < cameraIds_.size(); ++idx) {
		try {
			// Only publish/log a frame if the image source actually captured
			// a genuinely new one since the last cycle. Otherwise (e.g.
			// camera temporarily disconnected, or capture is simply slower
			// than this loop's cycle rate) skip this camera for this cycle
			// instead of repeatedly re-sending/re-logging the same stale
			// frame.
			if (!imgSources_[idx]->hasNewFrame()) {
				continue;
			}

			auto frame = imgSources_[idx]->getLatestFrame();
			const auto receiveTimestamp = std::chrono::system_clock::now();
			imagePublisher_.sendFrame(cameraIds_[idx], frame, receiveTimestamp);
			LOG_INFO("Image frame sent via UDP: cameraId={}, frameId={}, width={}, height={}, type={}",
				cameraIds_[idx], frame.frameId, frame.image.cols, frame.image.rows, frame.image.type());
		}
		catch (const std::exception& ex) {
			LOG_ERROR("LiveImagePassthrough cycle failed for camera {}: {}", cameraIds_[idx], ex.what());
		}
	}
}
