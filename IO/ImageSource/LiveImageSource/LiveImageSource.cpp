#include "LiveImageSource.h"

#include "Logger.h"


// Constructor: 
LiveImageSource::LiveImageSource(std::unique_ptr<IVideoStream> stream) : stream_(std::move(stream))
{
	LOG_TRACE("LiveImageSource created with provided video stream");
}

LiveImageSource::~LiveImageSource()
{
	LOG_TRACE("LiveImageSource is being destroyed, stopping stream if running");
	
	if (running_) {
		LOG_TRACE("LiveImageSource is still running during destruction, stopping it now...");
		stop();
	}
}

bool LiveImageSource::start()
{
	LOG_TRACE("Starting LiveImageSource...");
	if (running_) 
	{
		LOG_WARN("LiveImageSource is already running, start() call ignored");
		return false; // already running
	}

	LOG_TRACE("Opening video stream...");
	if (!stream_->open()) {
		LOG_ERROR("Failed to open video stream in LiveImageSource");
		return false;
	}

	running_ = true;

	// Start capture loop in a separate thread
	LOG_TRACE("Starting capture thread...");
	captureThread_ = std::thread(&LiveImageSource::captureLoop, this);

	return true;
}

bool LiveImageSource::stop()
{
	LOG_TRACE("Stopping LiveImageSource...");

	if (!running_) 
	{
		LOG_WARN("LiveImageSource is not running, stop() call ignored");
		return false; // already stopped
	}

	running_ = false;

	LOG_TRACE("Stopping capture thread...");

	if (captureThread_.joinable()) {
		captureThread_.join();
		LOG_TRACE("Capture thread stopped successfully");
	}
	
	if (!stream_->close()) {
		LOG_ERROR("Failed to close video stream in LiveImageSource");
		return false;
	}

	LOG_TRACE("Video stream closed successfully");
	return true;
}

ImageFrame LiveImageSource::getLatestFrame()
{
	LOG_TRACE("Getting latest frame from LiveImageSource...");
	std::scoped_lock lock(frameMutex_);

	if (!running_) {
		LOG_ERROR("LiveImageSource is not running, cannot get latest frame");
		throw std::runtime_error("LiveImageSource is not running");
	}

	LOG_TRACE("Latest frame retrieved successfully");
	return latestFrame_;
}

void LiveImageSource::captureLoop()
{
	LOG_TRACE("Starting capture loop in LiveImageSource...");

	// Calculate frame period based on config value maxCaptureFPS to control capture rate
	using clock = std::chrono::steady_clock;
	double maxFPS = stream_->getMaxCaptureFPS();
	const auto framePeriod = std::chrono::duration_cast<clock::duration>(
		std::chrono::duration<double>(1.0 / maxFPS));

	while (running_) {

		auto start = clock::now();

		ImageFrame frame = stream_->getFrame();

		// only assign latest frame if captured frame is not empty
		if (frame.image.empty()) {
			LOG_WARN("Captured empty frame in LiveImageSource, skipping...");

			// Sleep for the remaining frame period to control capture rate even when frames are empty
			auto elapsed = clock::now() - start;

			if (elapsed < framePeriod) {
				LOG_TRACE("Sleep for remaining frame period after capturing empty frame in LiveImageSource...");
				std::this_thread::sleep_for(framePeriod - elapsed);
			}

			continue;
		}

		{
			std::scoped_lock lock(frameMutex_);
			latestFrame_ = frame;
			//latestFrame_.image = frame.image.clone();

			// save image frame for debugging
			/*auto t = std::chrono::system_clock::to_time_t(frame.timestamp);
			std::stringstream filename;
			filename << "frame_" << t << ".png";

			cv::imwrite(filename.str(), frame.image);*/
		}
		LOG_TRACE("New frame captured and stored in LiveImageSource");

		auto elapsed = clock::now() - start;

		if (elapsed < framePeriod) {
			LOG_TRACE("Sleep for remaining frame period after capturing frame in LiveImageSource...");
			std::this_thread::sleep_for(framePeriod - elapsed);
		}
		LOG_TRACE("Current frame time");
	}
}